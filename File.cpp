#include "File.hpp"

#include <stdexcept>


/**
 * constructors
 */
File::File(const char* path, OpenMode mode/* = Mode::Read*/)
{
    open(path, mode);
}
File::File(const std::string& path, OpenMode mode/* = Mode::Read*/)
{
    open(path.c_str(), mode);
}
/**
 * descructor
 */
File::~File()
{
    close();
}

/**
 * explicitly open new file
 */
void File::open(const char* path, OpenMode mode/* = Mode::Read*/)
{
    //try to open file as a directory
    m_dir_handle = opendir(path);
    
    if(m_dir_handle != nullptr)
    {
        m_path = std::string(path);
        if(m_path[m_path.size() - 1] == '/' || m_path[m_path.size() - 1] == '\\')
        {
            m_path.pop_back();
        }
        return;
    }
    
    //try to open file as a normal file
    m_file_handle = fopen(path, mode == OpenMode::Read ? "rb" : "wb");
    
    if(m_file_handle == nullptr)
    {
        //TODO: error
        return;
    }
    
    m_path = std::string(path);
    if(m_path[m_path.size() - 1] == '/' || m_path[m_path.size() - 1] == '\\')
    {
        m_path.pop_back();
    }
}
void File::open(const std::string& path, OpenMode mode/* = Mode::Read*/)
{
    open(path.c_str(), mode);
}

/**
    * explicitly close file
    */
void File::close()
{
    if(m_dir_handle != nullptr)
    {
        closedir(m_dir_handle);
        m_dir_handle = nullptr;
    }
    if(m_file_handle != nullptr)
    {
        fclose(m_file_handle);
        m_file_handle = nullptr;
    }
    m_path.clear();
}

/**
    * get file size
    */
u64 File::size()
{
    if(m_file_handle == nullptr)
    {
        return 0;
    }
    
    u64 backup_pos = ftell(m_file_handle);
    fseek(m_file_handle, 0, SEEK_END);
    u64 file_size  = ftell(m_file_handle);
    
    // restore file pos
    fseek(m_file_handle, static_cast<long>(backup_pos), SEEK_SET);
    
    return file_size;
}

/**
    * read portion of file
    *
    * @arg num_bytes if set to 0, function will read whole file
    */
std::vector<u8> File::read(u64 num_bytes/* = 0*/)
{
    std::vector<u8> result;
    
    if(m_file_handle == nullptr)
    {
        throw std::runtime_error("File::read() error: file not opened");
    }
    
    // read whole file
    if(num_bytes == 0)
    {
        fseek(m_file_handle, 0, SEEK_SET);
        
        u64 result_size = this->size();
        
        if(result_size > 0xFFFFFF)
        {
            //TODO: warning, we are probably doing something wrong
        }
        
        result.resize(static_cast<u32>(result_size));
        fread(&result[0], 1, static_cast<size_t>(result_size), m_file_handle);
        
        fseek(m_file_handle, 0, SEEK_SET);
    }
    else
    {
        result.resize(static_cast<u32>(num_bytes));
        fread(&result[0], 1, static_cast<size_t>(num_bytes), m_file_handle);
    }

    return result;
}

u64 File::read(void* destination, u64 num_bytes)
{
    if(m_file_handle == nullptr)
    {
        throw std::runtime_error("File::read() error: file not opened");
    }
    
    u8* dest = reinterpret_cast<u8*>(destination);
    u64 bytes_read = 0;
    
    if(num_bytes == 0)
    {
        fseek(m_file_handle, 0, SEEK_SET);
        u64 result_size = this->size();
        
        bytes_read = fread(dest, 1, result_size, m_file_handle);
        fseek(m_file_handle, 0, SEEK_SET);
    }
    else
    {
        bytes_read = fread(dest, 1, num_bytes, m_file_handle);
    }
    
    return bytes_read;
}

/**
    * read text
    */
std::string File::read_text(u64 num_bytes/* = 0*/)
{
    auto&& data = read(num_bytes);

    return std::string(data.begin(), data.end());
}

/**
    * read 1 byte
    *
    * @return read byte or EOF
    */
int File::read_byte()
{
    if(m_file_handle == nullptr)
    {
        return File::Eof;
    }
    
    return fgetc(m_file_handle);
}

/**
    * write data itno a file
    */
u64 File::write(const char* data, u64 size)
{
    if(failed())
    {
        //TODO: error
        return 0;
    }
    
    return fwrite(data, 1, static_cast<size_t>(size), m_file_handle);
}
u64 File::write(const std::string& data)
{
    return write(data.c_str(), data.size());
}
u64 File::write(const std::string&& data)
{
    return write(data.c_str(), data.size());
}
u64 File::write(const std::vector<u8>& data)
{
    return write((const char*)&data[0], data.size());
}
    
/**
    * write 1 byte
    */
u64 File::write_byte(u8 byte)
{
    return write((const char*)&byte, 1);
}

/**
    * if we opened a directory we can list all file paths inside
    */
std::vector<std::string> File::list()
{
    std::vector<std::string> result;
    
    if(m_dir_handle == nullptr)
    {
        return result;
    }
    
    dirent* entry = nullptr;
    
    while((entry = readdir(m_dir_handle)))
    {
        result.emplace_back(entry->d_name);
    }
    
    return result;
}









