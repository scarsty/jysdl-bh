#include "ZipFile.h"
#include "zip.h"
#include "PotConv.h"


ZipFile::ZipFile()
{
}

ZipFile::~ZipFile()
{
    if (zip_)
    {
        zip_close(zip_);
    }
}

void ZipFile::open(const std::string& zip_filename)
{
    if (zip_)
    {
        zip_close(zip_);
    }
    auto u8name = PotConv::conv(zip_filename, "", "utf-8");
    //auto u8name = strcvt::CvtStringToUTF8(zip_filename);
    zip_ = zip_open(u8name.c_str(), ZIP_RDONLY, NULL);
}

void ZipFile::setPassword(const std::string& password) const
{
    if (zip_)
    {
        zip_set_default_password(zip_, password.c_str());
    }
}

std::string ZipFile::readFile(const std::string& filename) const
{
    std::string content;
    zip_file_t* zip_file;
    struct zip_stat zs;
    if (zip_)
    {
        std::lock_guard<std::mutex> lock(*mutex_);
        zip_file = zip_fopen(zip_, filename.c_str(), ZIP_FL_UNCHANGED);
        if (zip_file != NULL)
        {
            zip_stat_init(&zs);
            zip_stat(zip_, filename.c_str(), ZIP_FL_UNCHANGED, &zs);
            content.resize(zs.size);
            zip_fread(zip_file, content.data(), zs.size);
            zip_fclose(zip_file);
        }
    }
    return content;
}

std::vector<std::string> ZipFile::getFileNames() const
{
    std::vector<std::string> files;
    if (zip_)
    {
        int i, n = zip_get_num_entries(zip_, ZIP_FL_UNCHANGED);
        for (i = 0; i < n; ++i)
        {
            const char* name = zip_get_name(zip_, i, ZIP_FL_UNCHANGED);
            files.push_back(name);
        }
    }
    return files;
}

int ZipFile::zip(std::string zip_file, std::vector<std::string> files)
{
    zip_t* zip = zip_open(zip_file.c_str(), ZIP_CREATE | ZIP_TRUNCATE, NULL);
    for (auto& file : files)
    {
        std::string content;
        FILE* fp = fopen(file.c_str(), "rb");
        if (fp)
        {
            fseek(fp, 0, SEEK_END);
            content.resize(ftell(fp));
            fseek(fp, 0, SEEK_SET);
            fread((void*)content.data(), 1, content.size(), fp);
            fclose(fp);
            zip_source_t* source = zip_source_buffer_create(content.data(), content.size(), 0, NULL);
            zip_file_add(zip, file.c_str(), source, ZIP_FL_OVERWRITE); 
            zip_source_close(source);
        }
    }
    zip_close(zip);
    return 0;
}

int ZipFile::unzip(std::string zip_file, std::vector<std::string> files)
{
    ZipFile zip;
    zip.open(zip_file);
    for (auto& file : files)
    {
        std::string content = zip.readFile(file);
        FILE* fp = fopen(file.c_str(), "wb");
        if (fp)
        {
            fwrite(content.data(), 1, content.size(), fp);
            fclose(fp);
        }
    }
    return 0;
}
