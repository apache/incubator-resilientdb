
#include "address.pb.h"
#include <string>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

int main(){
	Address address;
	address.set_address("hello");

	string str;
	address.SerializeToString(&str);

	int fd = open("encode_file",O_RDWR|O_CREAT, 0666);
	assert(fd>=0);
	int len = write(fd, str.c_str(), str.size());
	close(fd);
}
