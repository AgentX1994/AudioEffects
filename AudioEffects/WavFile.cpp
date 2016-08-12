//
//  WavFile.cpp
//  WavFileOpener
//
//  Created by John Asper on 2016/8/10.
//  Copyright © 2016 John Asper. All rights reserved.
//

#include "WavFile.hpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>

// Sets/Resets all fields to zero
void WavFile::init(){
    samples = NULL;
    format = 0;
    num_channels = 0;
    sample_rate = 0;
    byte_rate = 0;
    block_align = 0;
    bits_per_sample = 0;
    num_samples = 0;
}

// Default Constructor
WavFile::WavFile(){
    init();
}

// Constructor
// Loads specified wav file into memory
WavFile::WavFile(std::string path){
    open(path);
}

// Frees samples if needed
// Outside of destructor so that open function can call it
void WavFile::freeSamples(){
    if(samples){
        for (int i = 0; i < num_channels; ++i) {
            delete [] samples[i];
        }
        delete [] samples;
    }
}


// Destructor
// Automatically deallocates any allocated memory
WavFile::~WavFile(){
    freeSamples();
}

// Known chunk id's of RIFF chunks
enum class WavChunks{
    RiffHeader = 0x52494646,
    Format = 0x666D7420,
    Data = 0x64617461
};

// Known formats of the wFormatTag field
enum class WavFormat {
    PulseCodeModulation = 0x01,
    IEEEFloatingPoint = 0x03,
    ALaw = 0x06,
    MuLaw = 0x07,
    IMAADPCM = 0x11,
    YamahaITUG723ADPCM = 0x16,
    GSM610 = 0x31,
    ITUG721ADPCM = 0x40,
    MPEG = 0x50,
    Extensible = 0xFFFE
};

// Subtype GUIDs
const unsigned char KSDATAFORMAT_SUBTYPE_PCM[] = {
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x10,
    0x00,
    0x80,
    0x00,
    0x00,
    0xaa,
    0x00,
    0x38,
    0x9b,
    0x71};

// Compares subtypes of the WAVE_FORMAT_EXTENSIBLE
bool compareSubtype(const unsigned char a[16], const unsigned char b[16]){
    for(int i = 0; i < 16; ++i){
        if(a[i] != b[i])
            return false;
    }
    return true;
}


// Normalizes the samples over the entire file
// sample/max_sample for all samples
void WavFile::normalizeSamples(){
    float max_sample = 0;
    
    for(int channel = 0; channel < num_channels; ++channel){
        for(int sample = 0; sample < num_samples; ++sample){
            if(max_sample < std::abs(samples[channel][sample])){
                
                // absolute value, or else the negative side of the spectrum will not be taken into account
                
                max_sample = std::abs(samples[channel][sample]);
            }
        }
    }
    
    std::cout << "Max Sample = " << std::setprecision(10) << max_sample << ", normalizing..." << std::endl << std::endl;
    
    for(int channel = 0; channel < num_channels; ++channel){
        for(int sample = 0; sample < num_samples; ++sample){
            samples[channel][sample] /= max_sample;
        }
    }
}

// Turns a 3 byte char array into a 32 bit int
// The ternary operator decides if sign extension is necessary
inline int32_t int24to32(unsigned char *in){
    return ((in[2] & 0x80) ? (0xff <<24) : 0) | (in[2] << 16) | (in[1] << 8) | in[0];
}

// Normalizing factors for conversions
const float uint8normalize = 2.0f/0xff; // Maps to [0,2], subtract 1 afterwards!
const float int16normalize = 1.0f/0x7fff;
const float int24normalize = 1.0f / 8388607.0; // Magic number, maps smallest to -1 and largest to 1

// Open a new wav file
// Deallocates old file if necessary
void WavFile::open(std::string path){
    
    // If a file is already loaded, free it
    freeSamples();
    init();
    
    char sep = '/';
    
#ifdef _WIN32
    sep = '\\'
#endif
    
    unsigned long i = path.rfind(sep);
    
    if (i != std::string::npos){
        filename = path.substr(i+1, path.length()-1);
    } else {
        filename = path;
    }
    
    // Open the file
    std::ifstream f;
    f.open(path, std::ios::binary);
    if(!f.is_open()){
        std::cerr << "Error: " << strerror(errno) << std::endl;
        throw std::runtime_error("WavFile Error: Could not open file\n");
    }
    
    // While not at end of file
    while(true){
        uint32_t chunkid;
        // The best way I have currently found to extract data fields
        f.read(reinterpret_cast<char*>(&chunkid), sizeof(chunkid));
        
        // Read will return garbage when reading past the end of file
        // If End Of File flag set, break;
        if(f.eof())
            break;
        
        // Chunk ID's are stored in big endian format, swap the bytes around
        chunkid = __builtin_bswap32(chunkid);
        switch((WavChunks)chunkid){
                
            case WavChunks::RiffHeader:
                // The header of the RIFF structure
                // Structure:
                // 4 bytes chunk size (filesize - 8 bytes)
                // 4 bytes format (must be 'WAVE' in big endian)
                
                f.read(reinterpret_cast<char*>(&filesize), sizeof(filesize));
                
                uint32_t format_specifier;
                f.read(reinterpret_cast<char*>(&format_specifier), sizeof(format_specifier));
                
                if (__builtin_bswap32(format_specifier) != 0x57415645) { // 0x57415645 is 'WAVE' stored in big endian
                    throw std::runtime_error("WavFile Error: Not a Wave File!");
                }
                break;
                
            case WavChunks::Format:
                // Format Subchunk specifying the format of the wave file
                // Structure:
                // 4 byte chunk size
                // 2 byte format tag
                // 2 byte number of channels
                // 4 byte sample rate
                // 4 byte byte rate
                // 2 byte block align
                // 2 byte bits per sample
                // ---- Optional extensions (if format tag is 0xFFFE)
                // 2 byte extra params size
                // 2 byte valid bits per sample
                // 4 byte channel mask
                // 16 byte subformat
                
                uint32_t chunksize;
                f.read(reinterpret_cast<char*>(&chunksize), sizeof(chunksize));;
                
                f.read(reinterpret_cast<char*>(&format), sizeof(format));
                
                f.read(reinterpret_cast<char*>(&num_channels), sizeof(num_channels));
                f.read(reinterpret_cast<char*>(&sample_rate), sizeof(sample_rate));
                f.read(reinterpret_cast<char*>(&byte_rate), sizeof(byte_rate));
                f.read(reinterpret_cast<char*>(&block_align), sizeof(block_align));
                f.read(reinterpret_cast<char*>(&bits_per_sample), sizeof(bits_per_sample));
                
                if ((WavFormat)format == WavFormat::Extensible){
                    uint16_t extra_params_size;
                    f.read(reinterpret_cast<char*>(&extra_params_size), sizeof(extra_params_size));
                    uint16_t valid_bits_per_sample;
                    f.read(reinterpret_cast<char*>(&valid_bits_per_sample), sizeof(valid_bits_per_sample));
                    uint32_t channel_mask;
                    f.read(reinterpret_cast<char*>(&channel_mask), sizeof(channel_mask));
                    unsigned char subformat[16];
                    f.read((char*)subformat, 16);
                    
                    if(compareSubtype(subformat, KSDATAFORMAT_SUBTYPE_PCM)){
                        std::cout << "Subformat is KSDATAFORMAT_SUBTYPE_PCM" << std::endl;
                    }
                }
                
                break;
                
            case WavChunks::Data:
                // Data Subchunk that stores the data
                // Structure:
                // 4 byte datasize
                // x bytes data
                
                uint32_t datasize;
                f.read(reinterpret_cast<char*>(&datasize), sizeof(datasize));
                samples = new float*[num_channels];
                num_samples =datasize*8/num_channels/bits_per_sample; // calculate number of samples
                
                for (int channel = 0; channel < num_channels; ++channel) {
                    samples[channel] = new float[num_samples];
                }
                
                // For linear PCM data:
                // Data is stored as a sequence of packets
                // each packet contains one sample for all channels
                
                for (int sample = 0; sample < num_samples; ++sample) {
                    for(int channel = 0; channel < num_channels; ++channel){
                        if (bits_per_sample == 8) {
                            uint8_t temp8bit;
                            f.read(reinterpret_cast<char*>(&temp8bit), sizeof(temp8bit));
                            
                            // Subtract one because the normalization factor maps to [0,2] and not [-1,1]
                            samples[channel][sample] = uint8normalize*(float)temp8bit - 1;
                            
                        } else if (bits_per_sample == 16) {
                            int16_t temp16bit;
                            f.read(reinterpret_cast<char*>(&temp16bit), sizeof(temp16bit));
                            
                            samples[channel][sample] = int16normalize*(float)temp16bit;
                        } else if (bits_per_sample == 24) {
                            
                            // There is no 24 bit variable in c++, so must use a 3 byte unsigned char array
                            unsigned char temp24bit[3] = {0,0,0};
                            f.read((char*)temp24bit, 3);
                            
                            int32_t temp = int24to32(temp24bit); // Convert the unsigned char into a 32-bit int
                            samples[channel][sample] = int24normalize*(float)temp; // Convert 32-bit int to float
                        }
                    }
                }
                break;
                
            default:
                // Some other chunk that we don't handle, just log it
                // Chunk IDs are coded plain text, convert it into a char array for output
                unsigned char tag[4];
                tag[0] = (chunkid >> 24) & 0xff;
                tag[1] = (chunkid >> 16) & 0xff;
                tag[2] = (chunkid >> 8) & 0xff;
                tag[3] = chunkid & 0xff;
                std::cout << "Encountered unknown chunk, ID: " << tag << " or " << std::hex << chunkid;
                std::cout << std::dec << " at byte " << f.tellg() << std::endl << std::endl;
                
                // Now just skip the chunk's data and go on
                uint32_t skipsize;
                f.read(reinterpret_cast<char*>(&skipsize), sizeof(skipsize));
                f.ignore(static_cast<int>(skipsize));
        }
    }
}

// Getters
uint16_t WavFile::getFormat(){
    return format;
}

uint16_t WavFile::getNumChannels(){
    return num_channels;
}

uint32_t WavFile::getSampleRate(){
    return sample_rate;
}

uint32_t WavFile::getByteRate(){
    return byte_rate;
}

uint16_t WavFile::getBlockAlign(){
    return block_align;
}

uint16_t WavFile::getBitsPerSample(){
    return bits_per_sample;
}

uint32_t WavFile::getNumSamples(){
    return num_samples;
}

float ** WavFile::getData(){
    return samples;
}

// Convert the format id into a string for display purposes
std::string audioFormatToString(WavFormat n){
    switch (n) {
        case WavFormat::PulseCodeModulation:
            return std::string("Linear PCM");
            break;
        case WavFormat::IEEEFloatingPoint:
            return std::string("IEEEFloating Point");
            break;
        case WavFormat::ALaw:
            return std::string("ALaw");
            break;
        case WavFormat::MuLaw:
            return std::string("MuLaw");
            break;
        case WavFormat::IMAADPCM:
            return std::string("IMAAD PCM");
            break;
        case WavFormat::YamahaITUG723ADPCM:
            return std::string("Yamaha ITUG723AD PCM");
            break;
        case WavFormat::GSM610:
            return std::string("GSM 610");
            break;
        case WavFormat::ITUG721ADPCM:
            return std::string("ITUG721AD PCM");
            break;
        case WavFormat::MPEG:
            return std::string("MPEG");
            break;
        case WavFormat::Extensible:
            return std::string("Extensible");
        default:
            return std::string("Unknown");
            break;
    }
}

// Pretty print runtime
std::string WavFile::printRuntime(){
    float runtime = (float)num_samples/(float)sample_rate;
    int seconds = (int)roundf(runtime);
    int minutes = seconds/60;
    seconds = seconds%60;
    std::stringstream s;
    s << minutes << "m " << seconds << "s";
    return s.str();
}

// Pretty print the Wave File details
std::string WavFile::toString(){
    std::stringstream s;
    s << "-Wave File-" << std::endl;
    s << "\tFileName: " << filename << std::endl;
    s << "\tSample Rate = " << sample_rate << " Hz" << std::endl;
    s << "\tAudio Format = " << audioFormatToString((WavFormat)format) << std::endl;
    s << "\tNumber of Channels = " << num_channels << std::endl;
    s << "\tByte Rate = " << byte_rate << std::endl;
    s << "\tBlock Align = " << block_align << std::endl;
    s << "\tBits per Sample = " << bits_per_sample << std::endl;
    s << "\tNumber of Samples = " << num_samples << std::endl;
    s << "\tRuntime = " << printRuntime() << std::endl << std::endl;
    return s.str();
}