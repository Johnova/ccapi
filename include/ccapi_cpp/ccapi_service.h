#ifndef INCLUDE_CCAPI_CPP_CCAPI_SERVICE_H_
#define INCLUDE_CCAPI_CPP_CCAPI_SERVICE_H_
namespace ccapi {
class Service {
 public:
  typedef wspp::lib::shared_ptr<ServiceContext> ServiceContextPtr;
  Service(std::function<void(Event& event)> eventHandler, SessionOptions sessionOptions,
                                      SessionConfigs sessionConfigs, ServiceContextPtr serviceContextPtr)
      :
        eventHandler(eventHandler),
        sessionOptions(sessionOptions),
        sessionConfigs(sessionConfigs),serviceContextPtr(serviceContextPtr)
  {
  }
  virtual ~Service() {
  }
  virtual void sendRequest(const Request& request, bool block) {}
  void setEventHandler(const std::function<void(Event& event)>& eventHandler) {
    this->eventHandler = eventHandler;
  }

 protected:
  std::function<void(Event& event)> eventHandler;
  SessionOptions sessionOptions;
  SessionConfigs sessionConfigs;
  ServiceContextPtr serviceContextPtr;
};
} /* namespace ccapi */
#endif  // INCLUDE_CCAPI_CPP_CCAPI_SERVICE_H_
