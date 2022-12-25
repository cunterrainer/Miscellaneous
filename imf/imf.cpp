#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"


class Image
{
private:
    int m_Width = 0;
    int m_Height = 0;
    int m_Channels = 0;
    unsigned char* m_Data = NULL;
public:
    Image(const char* filepath)
    {
        m_Data = stbi_load(filepath, &m_Width, &m_Height, &m_Channels, 0);
        if(m_Data == NULL)
        {
            std::cerr << "Failed to load image [" << filepath << "]\n";
            return;
        }
    }


    ~Image()
    {
        // missing copy / move constructor
        //if(m_Data != NULL)
        //    stbi_image_free(m_Data);
    }


    bool HasAlpha() const { return m_Channels == 4; }
    size_t Values() const { return m_Width * m_Height * m_Channels; }
    int  Channels() const { return m_Channels; }
    int  Height()   const { return m_Height;   }
    int  Width()    const { return m_Width;    }
    unsigned char& operator[](size_t index) const { return m_Data[index]; }


    void Save(const char* path) const
    {
        if(stbi_write_png(path, m_Width, m_Height, m_Channels, m_Data, m_Width * m_Channels) == 0)
            std::cerr << "Failed to write image [" << path << "]\n";
    }
};


namespace Filter
{
    Image Grey(const Image& img)
    {
        for(size_t i = 0; i < img.Values(); i += img.Channels())
        {
            size_t median = 0;
            size_t channels = img.HasAlpha() ? img.Channels() - 1 : img.Channels();
            for(size_t k = 0; k < channels; ++k)
                median += img[i + k];

            median /= channels;
            for(size_t k = 0; k < channels; ++k)
                img[i + k] = median;
        }
        return img;
    }


    Image FlipHorizontally(const Image& img)
    {
        for(size_t h = 0; h < img.Height(); ++h)
        {
            for(size_t w = 0; w < img.Width() / 2; ++w)
            {
                const size_t idx = (img.Width() * h + w) * img.Channels();
                const size_t EndIdx = (img.Width() * h + img.Width() - w - img.Channels() + img.Channels() - 1) * img.Channels();
                for(size_t c = 0; c < img.Channels(); ++c)
                    std::swap(img[idx + c], img[EndIdx + c]);
            }
        }
        return img;
    }


    Image FlipVertically(const Image& img)
    {
        for(size_t w = 0; w < img.Width(); ++w)
        {
            for(size_t h = 0; h < img.Height() / 2; ++h)
            {
                const size_t idx = (img.Width() * h + w) * img.Channels();
                const size_t EndIdx = img.Values() - img.Width() * img.Channels() * (h + 1) + w * img.Channels();
                for(size_t c = 0; c < img.Channels(); ++c)
                    std::swap(img[idx + c], img[EndIdx + c]);
            }
        }
        return img;
    }
}


int main()
{
    Image img("6.png");
    std::cout << "w:" << img.Width() << " h: " << img.Height() << " c: " << img.Channels() << std::endl;
    img = Filter::FlipVertically(img);
    img.Save("test.png");
}