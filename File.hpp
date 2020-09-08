#pragma once

#include "dirent.h"

#include <vector>
#include <string>
#include <cstdio>

#include "Types.hpp"

/**
* file and directory handler
*/
class File
{
public:
    
    /**
        * open mode
        */
    enum class OpenMode
    {
        Read,
        Write
    };
    
    /**
        * End of file identificator
        */
    constexpr static int Eof = EOF;
    
    /**
        * constructors
        */
    File(){}
    File(const char* path, OpenMode mode = OpenMode::Read);
    File(const std::string& path, OpenMode mode = OpenMode::Read);
    /**
        * descructor
        */
    ~File();
    
    /**
        * explicitly open new file
        */
    void open(const char* path, OpenMode mode = OpenMode::Read);
    void open(const std::string& path, OpenMode mode = OpenMode::Read);
    
    /**
        * explicitly close file
        */
    void close();
    
    /**
        * get file size
        */
    u64 size();
    
    /**
        * check if file is opened
        */
    bool opened() { return m_file_handle != nullptr || m_dir_handle != nullptr; }
    bool failed() { return m_file_handle == nullptr && m_dir_handle == nullptr; }
    
    /**
        * read raw portion of file
        *
        * @arg num_bytes if set to 0, function will read whole file
        */
    std::vector<u8> read(u64 num_bytes = 0);
    u64             read(void* destination, u64 num_bytes);

    /**
        * read text
        */
    std::string read_text(u64 num_bytes = 0);
    
    /**
        * read 1 byte
        *
        * @return read byte or EOF
        */
    int read_byte();
    
    /**
        * write data itno a file
        */
    u64 write(const char* data, u64 size);
    u64 write(const std::string& data);
    u64 write(const std::string&& data);
    u64 write(const std::vector<u8>& data);
    
    /**
        * write 1 byte
        */
    u64 write_byte(u8 byte);

    
    /**
        * did we open a file?
        */
    bool is_file() { return m_file_handle != nullptr; }
    /**
        * did we open a directory?
        */
    bool is_dir()  { return m_dir_handle  != nullptr; }
    
    /**
        * if we opened a directory we can list all file paths inside
        */
    std::vector<std::string> list();
    
    /**
        * get the path of the file
        */
    const std::string& get_path() { return m_path; }

    /**
        * get containing folder of the file
        */
    std::string get_folder()
    {
        std::string file_path = get_path();
        return file_path.substr(0, file_path.find_last_of("/\\"));
    }
    
private:
    
    FILE* m_file_handle { nullptr };
    DIR*  m_dir_handle  { nullptr };
    
    std::string m_path  { "" };
};
