
#include "address.pb.h"
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <glog/logging.h>

using namespace std;

const std::string test_dir = std::string(getenv("BUILD_WORKING_DIRECTORY")) + "/" ;

int main(){
	Address address;
	string str;

	string path = test_dir + std::string("encode_file");

	int fd = open(path.c_str(),O_CREAT|O_RDWR,0666);
	assert(fd>=0);
	char buf[1024];
	int len = read(fd, buf, sizeof(buf));
	close(fd);
	printf("fd = %d, len = %d\n",fd, len);

	address.ParseFromArray(buf, len);

	printf("decode:%s\n",address.address().c_str());
}
