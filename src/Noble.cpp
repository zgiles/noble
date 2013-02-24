// #include <v8.h>
// #include <node.h>

#import "BLEManager.h"

#include "Noble.h"

static v8::Persistent<v8::FunctionTemplate> s_ct;

class PeripheralDiscoveredData {
public:
  Noble::Noble *noble;
  std::string uuid;
  std::string localName;
  std::string mfgdata;
  std::vector<std::string> services;
  int rssi;
};

class PeripheralConnectedData {
public:
  Noble::Noble *noble;
  std::string uuid;
};

class PeripheralConnectFailureData {
public:
  Noble::Noble *noble;
  std::string uuid;
  std::string reason;
};

class PeripheralDisconnectedData {
public:
  Noble::Noble *noble;
  std::string uuid;
};

class PeripheralRssiUpdatedData {
public:
  Noble::Noble *noble;
  std::string uuid;
  int rssi;
};

class PeripheralServicesDiscoveredData {
public:
  Noble::Noble *noble;
  std::string uuid;
  std::vector<std::string> services;
};

void Noble::Init(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(Noble::New);

  s_ct = v8::Persistent<v8::FunctionTemplate>::New(t);
  s_ct->InstanceTemplate()->SetInternalFieldCount(1);
  s_ct->SetClassName(v8::String::NewSymbol("Noble"));

  NODE_SET_PROTOTYPE_METHOD(s_ct, "startScanning", Noble::StartScanning);
  NODE_SET_PROTOTYPE_METHOD(s_ct, "stopScanning", Noble::StopScanning);
  NODE_SET_PROTOTYPE_METHOD(s_ct, "connectPeripheral", Noble::ConnectPeripheral);
  NODE_SET_PROTOTYPE_METHOD(s_ct, "disconnectPeripheral", Noble::DisconnectPeripheral);
  NODE_SET_PROTOTYPE_METHOD(s_ct, "updatePeripheralRssi", Noble::UpdatePeripheralRssi);
  NODE_SET_PROTOTYPE_METHOD(s_ct, "discoverPeripheralServices", Noble::DiscoverPeripheralServices);

  target->Set(v8::String::NewSymbol("Noble"), s_ct->GetFunction());
}

Noble::Noble() : node::ObjectWrap(), state(StateUnknown) {
  this->bleManager = [[BLEManager alloc] initWithNoble:this];
}

Noble::~Noble() {
  [this->bleManager release];
}

void Noble::updateState(State state) {
  this->state = state;

  uv_work_t *req = new uv_work_t();
  req->data = this;

  uv_queue_work(uv_default_loop(), req, NULL, Noble::UpdateState);
}

void Noble::peripheralDiscovered(std::string uuid, std::string localName, std::string mfgdata, std::vector<std::string> services, int rssi) {
  uv_work_t *req = new uv_work_t();

  PeripheralDiscoveredData* data = new PeripheralDiscoveredData;

  data->noble = this;
  data->uuid = uuid;
  data->localName = localName;
  data->mfgdata = mfgdata;
  data->services = services;
  data->rssi = rssi;

  req->data = data;

  uv_queue_work(uv_default_loop(), req, NULL, Noble::PeripheralDiscovered);
}

void Noble::peripheralConnected(std::string uuid) {
  uv_work_t *req = new uv_work_t();

  PeripheralConnectedData* data = new PeripheralConnectedData;

  data->noble = this;
  data->uuid = uuid;

  req->data = data;

  uv_queue_work(uv_default_loop(), req, NULL, Noble::PeripheralConnected);
}

void Noble::peripheralConnectFailure(std::string uuid, std::string reason) {
  uv_work_t *req = new uv_work_t();

  PeripheralConnectFailureData* data = new PeripheralConnectFailureData;

  data->noble = this;
  data->uuid = uuid;
  data->reason = reason;

  req->data = data;

  uv_queue_work(uv_default_loop(), req, NULL, Noble::PeripheralConnectFailure);
}

void Noble::peripheralDisconnected(std::string uuid) {
  uv_work_t *req = new uv_work_t();

  PeripheralDisconnectedData* data = new PeripheralDisconnectedData;

  data->noble = this;
  data->uuid = uuid;

  req->data = data;

  uv_queue_work(uv_default_loop(), req, NULL, Noble::PeripheralDisonnected);
}

void Noble::peripheralRssiUpdated(std::string uuid, int rssi) {
  uv_work_t *req = new uv_work_t();

  PeripheralRssiUpdatedData* data = new PeripheralRssiUpdatedData;

  data->noble = this;
  data->uuid = uuid;
  data->rssi = rssi;

  req->data = data;

  uv_queue_work(uv_default_loop(), req, NULL, Noble::PeripheralRssiUpdated);
}

void Noble::peripheralServicesDiscovered(std::string uuid, std::vector<std::string> services) {
  uv_work_t *req = new uv_work_t();

  PeripheralServicesDiscoveredData* data = new PeripheralServicesDiscoveredData;

  data->noble = this;
  data->uuid = uuid;
  data->services = services;

  req->data = data;

  uv_queue_work(uv_default_loop(), req, NULL, Noble::PeripheralServicesDiscovered);
}

void Noble::startScanning(std::vector<std::string> services, bool allowDuplicates) {
  [this->bleManager startScanningForServices:services allowDuplicates:allowDuplicates];
}

void Noble::stopScanning() {
  [this->bleManager stopScanning];
}

void Noble::connectPeripheral(std::string uuid) {
  [this->bleManager connectPeripheral:uuid];
}

void Noble::disconnectPeripheral(std::string uuid) {
  [this->bleManager disconnectPeripheral:uuid];
}

void Noble::updatePeripheralRssi(std::string uuid) {
  [this->bleManager updatePeripheralRssi:uuid];
}

void Noble::discoverPeripheralServices(std::string uuid, std::vector<std::string> services) {
  [this->bleManager discoverPeripheral:uuid services:services];
}

v8::Handle<v8::Value> Noble::New(const v8::Arguments& args) {
  v8::HandleScope scope;
  Noble* p = new Noble();
  p->Wrap(args.This());
  p->This = v8::Persistent<v8::Object>::New(args.This());
  return args.This();
}

v8::Handle<v8::Value> Noble::StartScanning(const v8::Arguments& args) {
  v8::HandleScope scope;
  Noble* p = ObjectWrap::Unwrap<Noble>(args.This());

  std::vector<std::string> services;
  bool allowDuplicates = false;

  if (args.Length() > 0) {
    v8::Handle<v8::Value> arg0 = args[0];
    if (arg0->IsArray()) {
      v8::Handle<v8::Array> servicesArray = v8::Handle<v8::Array>::Cast(arg0);

      for(uint32_t i = 0; i < servicesArray->Length(); i++) {
        v8::Handle<v8::Value> serviceValue = servicesArray->Get(i);

        if (serviceValue->IsString()) {
          v8::String::AsciiValue serviceString(serviceValue->ToString());

          services.push_back(std::string(*serviceString));
        }
      }
    }
  }

  if (args.Length() > 1) {
    v8::Handle<v8::Value> arg1 = args[1];
    if (arg1->IsBoolean()) {
      allowDuplicates = arg1->ToBoolean()->Value();
    }
  }

  p->startScanning(services, allowDuplicates);

  v8::Handle<v8::Value> argv[1] = {
    v8::String::New("scanStart")
  };
  node::MakeCallback(args.This(), "emit", 1, argv);

  return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> Noble::StopScanning(const v8::Arguments& args) {
  v8::HandleScope scope;
  Noble* p = ObjectWrap::Unwrap<Noble>(args.This());
  p->stopScanning();

  v8::Handle<v8::Value> argv[1] = {
    v8::String::New("scanStop")
  };
  node::MakeCallback(args.This(), "emit", 1, argv);

  return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> Noble::ConnectPeripheral(const v8::Arguments& args) {
  v8::HandleScope scope;
  Noble* p = ObjectWrap::Unwrap<Noble>(args.This());

  std::string uuid;

  if (args.Length() > 0) {
    v8::Handle<v8::Value> arg0 = args[0];
    if (arg0->IsString()) {
      v8::String::AsciiValue serviceString(arg0->ToString());
      uuid = std::string(*serviceString);
    }
  }

  p->connectPeripheral(uuid);

  return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> Noble::DisconnectPeripheral(const v8::Arguments& args) {
  v8::HandleScope scope;
  Noble* p = ObjectWrap::Unwrap<Noble>(args.This());

  std::string uuid;

  if (args.Length() > 0) {
    v8::Handle<v8::Value> arg0 = args[0];
    if (arg0->IsString()) {
      v8::String::AsciiValue serviceString(arg0->ToString());
      uuid = std::string(*serviceString);
    }
  }

  p->disconnectPeripheral(uuid);

  return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> Noble::UpdatePeripheralRssi(const v8::Arguments& args) {
  v8::HandleScope scope;
  Noble* p = ObjectWrap::Unwrap<Noble>(args.This());

  std::string uuid;

  if (args.Length() > 0) {
    v8::Handle<v8::Value> arg0 = args[0];
    if (arg0->IsString()) {
      v8::String::AsciiValue serviceString(arg0->ToString());
      uuid = std::string(*serviceString);
    }
  }

  p->updatePeripheralRssi(uuid);

  return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> Noble::DiscoverPeripheralServices(const v8::Arguments& args) {
  v8::HandleScope scope;
  Noble* p = ObjectWrap::Unwrap<Noble>(args.This());

  std::string uuid;

  if (args.Length() > 0) {
    v8::Handle<v8::Value> arg0 = args[0];
    if (arg0->IsString()) {
      v8::String::AsciiValue serviceString(arg0->ToString());
      uuid = std::string(*serviceString);
    }
  }

  std::vector<std::string> services;

  if (args.Length() > 1) {
    v8::Handle<v8::Value> arg1 = args[1];
    if (arg1->IsArray()) {
      v8::Handle<v8::Array> servicesArray = v8::Handle<v8::Array>::Cast(arg1);

      for(uint32_t i = 0; i < servicesArray->Length(); i++) {
        v8::Handle<v8::Value> serviceValue = servicesArray->Get(i);

        if (serviceValue->IsString()) {
          v8::String::AsciiValue serviceString(serviceValue->ToString());

          services.push_back(std::string(*serviceString));
        }
      }
    }
  }

  p->discoverPeripheralServices(uuid, services);

  return scope.Close(v8::Undefined());
}

void Noble::UpdateState(uv_work_t* req) {
  v8::HandleScope scope;
  Noble* noble = static_cast<Noble*>(req->data);

  const char* state;

  switch(noble->state) {
    case StateResetting:
      state = "reset";
      break;

    case StateUnsupported:
      state = "unsupported";
      break;

    case StateUnauthorized:
      state = "unauthorized";
      break;

    case StatePoweredOff:
      state = "poweredOff";
      break;

    case StatePoweredOn:
      state = "poweredOn";
      break;

    case StateUnknown:
    default:
      state = "unknown";
      break;
  }

  v8::Handle<v8::Value> argv[2] = {
    v8::String::New("stateChange"),
    v8::String::New(state)
  };
  node::MakeCallback(noble->This, "emit", 2, argv);

  delete req;
}

void Noble::PeripheralDiscovered(uv_work_t* req) {
  v8::HandleScope scope;
  PeripheralDiscoveredData* data = static_cast<PeripheralDiscoveredData*>(req->data);
  Noble::Noble *noble = data->noble;

  v8::Handle<v8::Array> services = v8::Array::New();

  for (size_t i = 0; i < data->services.size(); i++) {
    services->Set(i, v8::String::New(data->services[i].c_str()));
  }

  v8::Handle<v8::Value> argv[6] = {
    v8::String::New("peripheralDiscover"),
    v8::String::New(data->uuid.c_str()),
    v8::String::New(data->localName.c_str()),
    v8::String::New(data->mfgdata.c_str()),
    services,
    v8::Integer::New(data->rssi)
  };
  node::MakeCallback(noble->This, "emit", 6, argv);

  delete data;
  delete req;
}

void Noble::PeripheralConnected(uv_work_t* req) {
  v8::HandleScope scope;
  PeripheralConnectedData* data = static_cast<PeripheralConnectedData*>(req->data);
  Noble::Noble *noble = data->noble;

  v8::Handle<v8::Value> argv[2] = {
    v8::String::New("peripheralConnect"),
    v8::String::New(data->uuid.c_str()),
  };
  node::MakeCallback(noble->This, "emit", 2, argv);

  delete data;
  delete req;
}

void Noble::PeripheralConnectFailure(uv_work_t* req) {
  v8::HandleScope scope;
  PeripheralConnectFailureData* data = static_cast<PeripheralConnectFailureData*>(req->data);
  Noble::Noble *noble = data->noble;

  v8::Handle<v8::Value> argv[3] = {
    v8::String::New("peripheralConnectFailure"),
    v8::String::New(data->uuid.c_str()),
    v8::String::New(data->reason.c_str())
  };
  node::MakeCallback(noble->This, "emit", 3, argv);

  delete data;
  delete req;
}

void Noble::PeripheralDisonnected(uv_work_t* req) {
  v8::HandleScope scope;
  PeripheralDisconnectedData* data = static_cast<PeripheralDisconnectedData*>(req->data);
  Noble::Noble *noble = data->noble;

  v8::Handle<v8::Value> argv[2] = {
    v8::String::New("peripheralDisconnect"),
    v8::String::New(data->uuid.c_str()),
  };
  node::MakeCallback(noble->This, "emit", 2, argv);

  delete data;
  delete req;
}

void Noble::PeripheralRssiUpdated(uv_work_t* req) {
  v8::HandleScope scope;
  PeripheralRssiUpdatedData* data = static_cast<PeripheralRssiUpdatedData*>(req->data);
  Noble::Noble *noble = data->noble;

  v8::Handle<v8::Value> argv[3] = {
    v8::String::New("peripheralRssiUpdate"),
    v8::String::New(data->uuid.c_str()),
    v8::Integer::New(data->rssi)
  };
  node::MakeCallback(noble->This, "emit", 3, argv);

  delete data;
  delete req;
}

void Noble::PeripheralServicesDiscovered(uv_work_t* req) {
  v8::HandleScope scope;
  PeripheralServicesDiscoveredData* data = static_cast<PeripheralServicesDiscoveredData*>(req->data);
  Noble::Noble *noble = data->noble;

  v8::Handle<v8::Array> services = v8::Array::New();

  for (size_t i = 0; i < data->services.size(); i++) {
    services->Set(i, v8::String::New(data->services[i].c_str()));
  }

  v8::Handle<v8::Value> argv[3] = {
    v8::String::New("peripheralServicesDiscover"),
    v8::String::New(data->uuid.c_str()),
    services
  };
  node::MakeCallback(noble->This, "emit", 3, argv);

  delete data;
  delete req;  
}

extern "C" {

  static void init (v8::Handle<v8::Object> target) {
    Noble::Init(target);
  }

  NODE_MODULE(binding, init);
}
