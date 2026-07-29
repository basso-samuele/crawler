#include <thread>

#include <InputStream.hpp>
#include <FileInputStream.hpp>

namespace Stream
{

void Producer(Crawler::FileInputStream& inputStream) {
    while (inputStream.ReadFromDisk());
}

void Consumer(Crawler::InputStream& inputStream) {
    std::byte b;
    while (!inputStream.Peek(&b)) {
        printf("%c", static_cast<char>(static_cast<uint8_t>(b)));
        inputStream.Drop();
    }
}

}

int main(int argc, char** argv) {
    Crawler::FileInputStream fileInputStream(CRAWLER_STREAM_ASSET_PATH);
    std::thread prod(Stream::Producer, std::ref(fileInputStream));
    std::thread cons(Stream::Consumer, std::ref(fileInputStream));
    prod.join();
    cons.join();
    return 0;
}