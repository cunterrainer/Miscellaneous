#include <iostream>
#include <vector>
#include <string_view>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// bright ------------------> dark
// "N@#W$9876543210?!abc;:+=-,._ "

class Image
{
private:
    int m_Width;
    int m_Height;
    int m_Channel;
    unsigned char* m_Data;
public:
    Image(const char* const path)
    {
        m_Data = stbi_load(path, &m_Width, &m_Height, &m_Channel, 0);
        if(m_Data == NULL) 
        {
            std::cerr << "Failed to load image [" << path << "]\n";
            return;
        }
    }


    ~Image()
    {
        stbi_image_free(m_Data);
    }


    int Width() const { return m_Width; }


    std::vector<unsigned char> ExtractBrightness() const
    {
        std::vector<unsigned char> brightness;
        for(int i = 0; i < m_Width * m_Height * m_Channel; i += m_Channel)
        {
            unsigned char average = (m_Data[i] + m_Data[i + 1] + m_Data[i + 2]) / 3;
            if(m_Channel == 4 && m_Data[i + 3] != 255)
                average = 0;
            brightness.push_back(average);
        }
        return brightness;
    }
};


int main()
{
    Image img("img.png");
    const std::vector<unsigned char> pixelBrightness = img.ExtractBrightness();

    const std::string_view asciiChars = "N@#W$9876543210?!abc;:+=-,._ ";
    const unsigned char asciiThreeshold = 255 / asciiChars.size();

    int w = 0;
    for(auto& p : pixelBrightness)
    {
        ++w;
        if(w == img.Width())
        {
            std::cout << '\n';
            w = 0;
        }
        const unsigned char charIdx = p % asciiThreeshold;
        std::cout << asciiChars[charIdx];
    }
}