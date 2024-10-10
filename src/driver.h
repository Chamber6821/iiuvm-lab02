#pragma once

enum OPERATION {
	READ_CODE = 0,
	WRITE_CODE = 1
};

struct IoRequestRead {
	unsigned short port;
};

struct IoRequestWrite {
	unsigned short port;
	unsigned int value;
};

struct IoResponse {
	unsigned int value;
};