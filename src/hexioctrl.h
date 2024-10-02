#include <string>

/*
  HexIOWrapper hwr;
  hwr.StartUp();
  hwr.AllowExclusiveAccess();
  hwr.ShutDown();
 */
class HexIOWrapper {
public:
  HexIOWrapper();
  virtual ~HexIOWrapper();
  bool StartUp();
  bool ShutDown();
  std::string GetStatus();

  unsigned char ReadPortUCHAR(unsigned char port);
  unsigned short ReadPortUSHORT(unsigned short port);
  unsigned long ReadPortULONG(unsigned long port);

  void WritePortUCHAR(unsigned char port, unsigned char value);
  void WritePortUSHORT(unsigned short port, unsigned short value);
  void WritePortULONG(unsigned long port, unsigned long value);

  bool AllowExclusiveAccess();
};
