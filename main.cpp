#include <iostream>
#include "string"
#include "limits"
#include <cmath>
#include <sys/socket.h>
#include <netdb.h>
#include <fstream>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <functional>

using namespace std;
#define BUFFER_LEN = 8192

class Http {
    typedef void (*DataCallback)(const char * data);
protected:
    const char* DEFAULT_METHOD = "GET";
    const char* DEFAULT_CONTENT_TYPE = "raw";
    DataCallback callback;
    string *_method = nullptr;
    string *contentType = nullptr;
    string *data = nullptr;
    string *url;

    string getHost(string uri) {
        return uri.substr(0, uri.find('/', 0));
    }

    string getBody(string uri) {
        signed long start = uri.find('/', 0);
        if (start < 0) {
            return "/";
        }
        return uri.substr(start, uri.length());
    }

    int getPort(){
        signed long pStart = this->url->find(':');
        if(pStart > 0){
            signed long pEnd = this->url->find('/', pStart);
            if(pEnd > 0){
                return atoi(this->url->substr(pStart, pEnd).c_str());
            } else{
                return atoi(this->url->substr(pStart, this->url->length()).c_str());
            }
        }
        return 80;
    }
private:
    int _fd;
    stringstream stream;
    void initConnect(){
        hostent* host = gethostbyname(getHost(this->url->c_str()).c_str());
        sockaddr_in sockIn = {
                .sin_family = AF_INET,
                .sin_port = htons(getPort())
        };
        memcpy(&sockIn.sin_addr.s_addr, host->h_addr, host->h_length);
        _fd = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(_fd, (sockaddr *) &sockIn, sizeof(sockIn)) == -1) {
            cout << "CONNECT FAILED" << endl;
        } else{
            cout << "CONNECT SUCCESSED" << endl;
        }
    }

    void writeContent(){
        if(_method)
            stream << _method;
        else
            stream << DEFAULT_METHOD;
        stream << " " << getBody(this->url->c_str()) << ' ' << "HTTP/1.1\r\n";
        stream << "Host: " << getHost(this->url->c_str()) << "\r\n";
        stream << "Content-Type: ";
        if(contentType)
            stream << contentType;
        else
            stream << DEFAULT_CONTENT_TYPE;
        stream << "\r\n";
        if(_method && _method->compare("POST") && data)
            stream << data << "\r\n";
        stream << "\r\n";
        auto str = stream.str();
        write(_fd, str.data(), str.size());
    }

    void readContent(){
        ssize_t result;
        auto buffer = new char[8192];
        bool body = false;
        stringstream head;
        do {
            memset(buffer, 0, 8192);
            result = read(_fd, buffer, 8192);
            if (buffer[result - 1] == '\n' && buffer[result - 2] == '\r') {
                close(_fd);
            }
            if(body && result >= 0){
                callback(buffer);
            }
            if(!body && result > 0){
                head.write(buffer, result);
                string s(buffer, result);
                signed long pos = s.find("\r\n\r\n", 0);
                if(pos > 0){
                    body = true;
                    callback(buffer + pos + 4);
                }
            }
        } while (result > 0);
        delete [] buffer;
    }

public:

    Http(string url) {
        this->url = new string(url);
    }

    Http(const char *url) {
        this->url = new string(url);
    }

    void setMethod(string method){
        this->_method = new string(method);
    }

    void setContentType(string ct){}

    void setContentData(string data){}

    void setCallback(DataCallback callback){
        this->callback = callback;
    }

    string *getUrl() {
        return url;
    }

    void exec(){
        initConnect();
        writeContent();
        readContent();
    }

    ~Http() {
        delete this->url;
        if(data)
            delete this->data;
        if(_method)
            delete this->_method;
    }
};


int main() {
    string c = "www.cqzk.com.cn";
    auto h = new Http(c);
    h->setCallback([](const char * data){
        cout << data << endl;
    });
    h->exec();
    return 0;
}

