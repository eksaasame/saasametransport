#pragma once
#ifndef aws_s3_H
#define aws_s3_H

#include <macho.h>

class aws_s3{
public:
    typedef boost::shared_ptr<aws_s3> ptr;
    typedef std::vector<ptr>          vtr;
    typedef std::deque<ptr>           queue;
    struct item{
        typedef boost::shared_ptr<item> ptr;
        typedef std::vector<ptr> vtr;
        item() : is_dir(false){}
        std::string name;
        bool        is_dir;
    };
    virtual bool create_dir(std::string _path) = 0;
    virtual bool remove_dir(std::string _path) = 0 ;
    virtual bool is_dir_existing(std::string _path) = 0;
    virtual bool is_existing(std::string _path) = 0;
    virtual bool get_file(std::string _path, std::ostream& data) = 0;
    virtual bool put_file(std::string _path, std::istream& data) = 0;
    virtual bool remove(std::string _path) = 0;
    virtual bool create_empty_file(std::string _path) = 0;
    virtual item::vtr enumerate_sub_items(std::string _path) = 0;
    virtual aws_s3::ptr clone() = 0;
    virtual std::string key_id() = 0;
    virtual std::string bucket() const = 0;
    static aws_s3::ptr connect(std::string bucket, std::string access_key_id, std::string secret_key, int region = 0, std::string proxy_host = "", uint32_t proxy_port = 0, std::string proxy_username = "", std::string proxy_password = "", int32_t timeout = 300);
    class lock_ex : virtual public macho::windows::lock_able_ex {
    public:
        lock_ex(boost::filesystem::path file, aws_s3::ptr s3, std::string flag, int wait_count = 10000) : _lock(false), _s3(s3), _flag(false), _count(wait_count){
            _lock_file = file.string() + ".lock/";
            if (flag.length())
                _flag_file = _lock_file + flag + "/";
        }
        ~lock_ex(){
            unlock();
        }
        bool lock(){
            _lock = _flag = true;
            return true;
        }
        bool unlock(){
            _lock = _flag = false;
            return true;
        }
        bool trylock(){
            _lock = _flag = true;
            return true;
        }
    private:
        std::string             _lock_file;
        aws_s3::ptr             _s3;
        bool                    _lock;
        std::string             _flag_file;
        bool                    _flag;
        int                     _count;
    };

};



#endif