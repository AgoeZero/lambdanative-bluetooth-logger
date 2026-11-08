
(c-declare #<<end-of-c-declare

#ifdef ANDROID
  const char * GetDeviceName(int);
  void ConnectToDevice(int);
  const char * ReadFromSocket();
#endif

end-of-c-declare
)

(define get-bluetooth-device-name (c-lambda(int) char-string "GetDeviceName"))
(define connect-to-bluetooth-device (c-lambda(int) void "ConnectToDevice"))
(define read-from-bluetooth-socket (c-lambda() char-string "ReadFromSocket"))