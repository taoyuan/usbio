#include "./bindings.h"

#define THROW_BAD_ARGS(fail) return ThrowException(Exception::TypeError(V8STR(fail)));
#define THROW_NOT_YET return ThrowException(Exception::TypeError(String::Concat(String::New(__FUNCTION__), String::New("not yet supported"))));
#define CHECK_USB(r, scope) \
	if (r < LIBUSB_SUCCESS) { \
		return scope.Close(ThrowException(errno_exception(r)));\
	}

#define LOCAL(type, varname, ref) \
		HandleScope scope;\
		type *varname = OBJUNWRAP<type>(ref);

/**
 * Remarks
 *  * variable name "self" always refers to the unwraped object instance of static method class
 *  * there is no node-usb support for libusbs context. Want to keep it simple.
 */

namespace NodeUsb {
/******************************* Helper functions */
	static inline Local<Value> errno_exception(int errorno) {
		Local<Value> e  = Exception::Error(String::NewSymbol(strerror(errorno)));
		Local<Object> obj = e->ToObject();
		std::string err = "";

		obj->Set(NODE_PSYMBOL("errno"), Integer::New(errorno));
		// taken from pyusb
		switch (errorno) {
			case LIBUSB_ERROR_IO:
				err = "Input/output error";
				break;
			case LIBUSB_ERROR_INVALID_PARAM:
				err  = "Invalid parameter";
				break;
			case LIBUSB_ERROR_ACCESS:
				err  = "Access denied (insufficient permissions)";
				break;
			case LIBUSB_ERROR_NO_DEVICE:
				err = "No such device (it may have been disconnected)";
				break;
			case LIBUSB_ERROR_NOT_FOUND:
				err = "Entity not found";
				break;
			case LIBUSB_ERROR_BUSY:
				err = "Resource busy";
				break;
			case LIBUSB_ERROR_TIMEOUT:
				err = "Operation timed out";
				break;
			case LIBUSB_ERROR_OVERFLOW:
				err = "Overflow";
				break;
			case LIBUSB_ERROR_PIPE:
				err = "Pipe error";
				break;
			case LIBUSB_ERROR_INTERRUPTED:
				err = "System call interrupted (perhaps due to signal)";
				break;
			case LIBUSB_ERROR_NO_MEM:
				err = "Insufficient memory";
				break;
			case LIBUSB_ERROR_NOT_SUPPORTED:
				err = "Operation not supported or unimplemented on this platform";
				break;
			default:
				err = "Unknown error";
				break;
		}
		// convert err to const char* with help of c_str()
		obj->Set(NODE_PSYMBOL("msg"), String::New(err.c_str()));
		return e;
	}

/******************************* USB */
	/**
	 * @param usb.isLibusbInitalized: boolean
	 */
	void Usb::Initalize(Handle<Object> target) {
		DEBUG("Entering")
		HandleScope  scope;
		
		Local<FunctionTemplate> t = FunctionTemplate::New(Usb::New);

		// Constructor
		t->Inherit(EventEmitter::constructor_template);
		t->InstanceTemplate()->SetInternalFieldCount(1);
		t->SetClassName(String::NewSymbol("Usb"));

		Local<ObjectTemplate> instance_template = t->InstanceTemplate();

		// Constants must be passed to ObjectTemplate and *not* to FunctionTemplate
		// libusb_class_node
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_PER_INTERFACE);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_AUDIO);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_COMM);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_HID);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_PRINTER);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_PTP);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_MASS_STORAGE);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_HUB);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_DATA);
		// both does not exist?
		// NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_WIRELESS);
		// NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_APPLICATION);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_VENDOR_SPEC);
		// libusb_descriptor_type
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_DEVICE);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_CONFIG);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_STRING);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_INTERFACE);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_ENDPOINT);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_HID);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_REPORT);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_PHYSICAL);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_HUB);
		// libusb_endpoint_direction
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ENDPOINT_IN);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ENDPOINT_OUT);
		// libusb_transfer_type
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_TYPE_CONTROL);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_TYPE_ISOCHRONOUS);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_TYPE_BULK);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_TYPE_INTERRUPT);
		// libusb_iso_sync_type
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ISO_SYNC_TYPE_NONE);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ISO_SYNC_TYPE_ASYNC);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ISO_SYNC_TYPE_ADAPTIVE);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ISO_SYNC_TYPE_SYNC);
		// libusb_iso_usage_type
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ISO_USAGE_TYPE_DATA);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ISO_USAGE_TYPE_FEEDBACK);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ISO_USAGE_TYPE_IMPLICIT);
		// libusb_transfer_status
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_COMPLETED);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_ERROR);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_TIMED_OUT);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_CANCELLED);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_STALL);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_NO_DEVICE);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_OVERFLOW);
		// libusb_transfer_flags
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_SHORT_NOT_OK);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_FREE_BUFFER);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_FREE_TRANSFER);

		// Properties
		// TODO: should get_device_list be a property?
		instance_template->SetAccessor(V8STR("isLibusbInitalized"), Usb::IsLibusbInitalizedGetter);

		// Bindings to nodejs
		NODE_SET_PROTOTYPE_METHOD(t, "refresh", Usb::Refresh);
		NODE_SET_PROTOTYPE_METHOD(t, "getDevices", Usb::GetDeviceList);
		NODE_SET_PROTOTYPE_METHOD(t, "close", Usb::Close);

		// Export Usb class to V8/node.js environment
		target->Set(String::NewSymbol("Usb"), t->GetFunction());
		DEBUG("Leave")
	}

	Usb::Usb() : EventEmitter() {
		is_initalized = false;
		num_devices = 0;
		devices = NULL;
	}

	Usb::~Usb() {
		if (devices != NULL) {
			libusb_free_device_list(devices, 1);
		}

		libusb_exit(NULL);
		DEBUG("NodeUsb::Usb object destroyed")
		// TODO Freeing opened USB devices?
	}

	/**
	 * Methods not exposed to v8 - used only internal
	 */
	int Usb::Init() {
		if (is_initalized) {
			return LIBUSB_SUCCESS;
		}

		int r = libusb_init(NULL);

		if (0 == r) {
			is_initalized = true;
		}

		return r;
	}
	
	/**
	 * Returns libusb initalization status 
	 * @return boolean
	 */
	Handle<Value> Usb::IsLibusbInitalizedGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Usb, self, info.Holder())
		
		if (self->is_initalized == true) {
			return scope.Close(True());
		}

		return scope.Close(False());
	}


	/**
	 * Creates a new Usb object
	 */
	Handle<Value> Usb::New(const Arguments& args) {
		LOCAL(Usb, self, args.This())

		// wrap self object to arguments
		self->Wrap(args.This());

		return args.This();
	}

	/**
	 * Refresh libusb environment
	 */
	Handle<Value> Usb::Refresh(const Arguments& args) {
		LOCAL(Usb, self, args.This())

		CHECK_USB(self->Init(), scope);
		return scope.Close(True());
	}


	/**
	 * Returns the devices discovered by libusb
	 * @return array[Device]
	 */
	Handle<Value> Usb::GetDeviceList(const Arguments& args) {
		LOCAL(Usb, self, args.This())

		CHECK_USB(self->Init(), scope);

		// dynamic array (sic!) which contains the devices discovered later
		Local<Array> discoveredDevices = Array::New();

		// TODO Google codeguide for ssize_t?
		ssize_t i = 0;

		// if no devices were covered => get device list
		if (self->devices == NULL) {
			DEBUG("Discover device list");
			self->num_devices = libusb_get_device_list(NULL, &(self->devices));
			CHECK_USB(self->num_devices, scope);
		}

		// js_device contains the Device instance
		Local<Object> js_device;

		for (; i < self->num_devices; i++) {
			// wrap libusb_device structure into a Local<Value>
			Local<Value> arg = External::New(self->devices[i]);

			// create new object instance of class NodeUsb::Device  
			Persistent<Object> js_device(Device::constructor_template->GetFunction()->NewInstance(1, &arg));
			
			// push to array
			discoveredDevices->Set(Integer::New(i), js_device);
		}

		return scope.Close(discoveredDevices);
	}

	/**
	 * Close current libusb context
	 * @return boolean
	 */
	Handle<Value> Usb::Close(const Arguments& args) {
		LOCAL(Usb, self, args.This())

		if (false == self->is_initalized) {
			return scope.Close(False());
		}

		delete self;

		return scope.Close(True());
	}

/******************************* Device */
	/** constructor template is needed for creating new Device objects from outside */
	Persistent<FunctionTemplate> Device::constructor_template;

	/**
	 * @param device.busNumber integer
	 * @param device.deviceAddress integer
	 */
	void Device::Initalize(Handle<Object> target) {
		DEBUG("Entering...")
		HandleScope  scope;
		Local<FunctionTemplate> t = FunctionTemplate::New(Device::New);

		// Constructor
		t->InstanceTemplate()->SetInternalFieldCount(1);
		t->SetClassName(String::NewSymbol("Device"));
		Device::constructor_template = Persistent<FunctionTemplate>::New(t);

		Local<ObjectTemplate> instance_template = t->InstanceTemplate();

		// Constants
		// no constants at the moment
	
		// Properties
		instance_template->SetAccessor(V8STR("deviceAddress"), Device::DeviceAddressGetter);
		instance_template->SetAccessor(V8STR("busNumber"), Device::BusNumberGetter);

		// Bindings to nodejs
		NODE_SET_PROTOTYPE_METHOD(t, "reset", Device::Reset); 
		NODE_SET_PROTOTYPE_METHOD(t, "getDeviceDescriptor", Device::GetDeviceDescriptor);
		NODE_SET_PROTOTYPE_METHOD(t, "getConfigDescriptor", Device::GetConfigDescriptor);

		// Make it visible in JavaScript
		target->Set(String::NewSymbol("Device"), t->GetFunction());	
		DEBUG("Leave")
	}

	Device::Device(libusb_device* _device) {
		DEBUG("Assigning libusb_device structure to self")
		device = _device;
		config_descriptor = NULL;
	}

	Device::~Device() {
		DEBUG("Device object destroyed")
	}

	Handle<Value> Device::New(const Arguments& args) {
		HandleScope scope;
		DEBUG("New Device object created")

		// need libusb_device structure as first argument
		if (args.Length() <= 0 || !args[0]->IsExternal()) {
			THROW_BAD_ARGS("Device::New argument is invalid. Must be external!") 
		}

		Local<External> refDevice = Local<External>::Cast(args[0]);

		// cast local reference to local libusb_device structure 
		libusb_device *libusbDevice = static_cast<libusb_device*>(refDevice->Value());

		// create new Device object
		Device *device = new Device(libusbDevice);

		// wrap created Device object to v8
		device->Wrap(args.This());

		return args.This();
	}
	
	/**
	 * @return integer
	 */
	Handle<Value> Device::BusNumberGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Device, self, info.Holder())
		uint8_t bus_number = libusb_get_bus_number(self->device);

		return scope.Close(Integer::New(bus_number));
	}

	/**
	 * @return integer
	 */
	Handle<Value> Device::DeviceAddressGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Device, self, info.Holder())
		uint8_t address = libusb_get_device_address(self->device);

		return scope.Close(Integer::New(address));
	}

	Handle<Value> Device::Reset(const Arguments& args) {
		HandleScope scope;
		THROW_NOT_YET
		return scope.Close(True());
	}


// TODO: Read-Only
#define LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(name) \
		r->Set(V8STR(#name), Integer::New((*self->config_descriptor).name));

	/**
	 * Returns configuration descriptor structure
	 */
	Handle<Value> Device::GetConfigDescriptor(const Arguments& args) {
		// make local value reference to first parameter
		Local<External> refDevice = Local<External>::Cast(args[0]);

		LOCAL(Device, self, args.This())
		CHECK_USB(libusb_get_active_config_descriptor(self->device, &(self->config_descriptor)), scope)
		Local<Object> r = Object::New();

		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bLength)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bDescriptorType)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(wTotalLength)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bNumInterfaces)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bConfigurationValue)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(iConfiguration)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bmAttributes)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(MaxPower)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(extra_length)

		Local<Array> interfaces = Array::New();
		
		// iterate interfaces
		for (int i = 0; i < (*self->config_descriptor).bNumInterfaces; i++) {
			libusb_interface interface_container = (*self->config_descriptor).interface[i];

			for (int j = 0; j < interface_container.num_altsetting; j++) {
				libusb_interface_descriptor interface_descriptor = interface_container.altsetting[j];

				Local<Value> args_new_interface[2] = {
					External::New(self->device),
					External::New(&interface_descriptor),
				};

				// create new object instance of class NodeUsb::Interface  
				Persistent<Object> js_interface(Interface::constructor_template->GetFunction()->NewInstance(2, args_new_interface));
				Local<Array> endpoints = Array::New();

				// interate endpoints
				for (int k = 0; k < interface_descriptor.bNumEndpoints; k++) {
					libusb_endpoint_descriptor endpoint_descriptor = interface_descriptor.endpoint[k];

					Local<Value> args_new_endpoint[2] = {
						External::New(self->device),
						External::New(&endpoint_descriptor),
					};

					// create new object instance of class NodeUsb::Endpoint
					Persistent<Object> js_endpoint(Endpoint::constructor_template->GetFunction()->NewInstance(2, args_new_endpoint));
					endpoints->Set(k, js_endpoint);
				}

				js_interface->Set(V8STR("endpoints"), endpoints);
				interfaces->Set(i, js_interface);
			}
		}
		
		r->Set(V8STR("interfaces"), interfaces);
		// free it
		libusb_free_config_descriptor(self->config_descriptor);

		return scope.Close(r);
	}
	
// TODO: Read-Only
#define LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(name) \
		r->Set(V8STR(#name), Integer::New(self->device_descriptor.name));

	/**
	 * Returns the device descriptor of current device
	 * @return object
	 */
	Handle<Value> Device::GetDeviceDescriptor(const Arguments& args) {
		LOCAL(Device, self, args.This())
		CHECK_USB(libusb_get_device_descriptor(self->device, &(self->device_descriptor)), scope)
		Local<Object> r = Object::New();

		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bLength)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bDescriptorType)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bcdUSB)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bDeviceClass)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bDeviceSubClass)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bDeviceProtocol)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bMaxPacketSize0)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(idVendor)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(idProduct)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bcdDevice)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(iManufacturer)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(iProduct)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(iSerialNumber)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bNumConfigurations)

		return scope.Close(r);
	}

/******************************* Interface */
	Persistent<FunctionTemplate> Interface::constructor_template;

	Interface::Interface(libusb_device* _device, libusb_interface_descriptor* _interface_descriptor) {
		DEBUG("Assigning libusb_device and libusb_interface_descriptor structure to self")
		device = _device;
		descriptor = _interface_descriptor;
	}

	Interface::~Interface() {
		// TODO Close
		DEBUG("Device object destroyed")
	}


	void Interface::Initalize(Handle<Object> target) {
		DEBUG("Entering...")
		HandleScope  scope;
		Local<FunctionTemplate> t = FunctionTemplate::New(Interface::New);

		// Constructor
		t->InstanceTemplate()->SetInternalFieldCount(1);
		t->SetClassName(String::NewSymbol("Interface"));
		Interface::constructor_template = Persistent<FunctionTemplate>::New(t);

		Local<ObjectTemplate> instance_template = t->InstanceTemplate();

		// Constants
		// no constants at the moment
	
		// Properties
		instance_template->SetAccessor(V8STR("__isKernelDriverAttached"), Interface::IsKernelDriverActiveGetter);

		// methods exposed to node.js

		// Make it visible in JavaScript
		target->Set(String::NewSymbol("Interface"), t->GetFunction());	
		DEBUG("Leave")
	}

	Handle<Value> Interface::New(const Arguments& args) {
		HandleScope scope;
		DEBUG("New Device object created")

		// need libusb_device structure as first argument
		if (args.Length() != 2 || !args[0]->IsExternal() || !args[1]->IsExternal()) {
			THROW_BAD_ARGS("Device::New argument is invalid. [object:external:libusb_device, object:external:libusb_interface_descriptor]!") 
		}

		// assign arguments as local references
		Local<External> refDevice = Local<External>::Cast(args[0]);
		Local<External> refInterfaceDescriptor = Local<External>::Cast(args[1]);

		libusb_device *libusbDevice = static_cast<libusb_device*>(refDevice->Value());
		libusb_interface_descriptor *libusbInterfaceDescriptor = static_cast<libusb_interface_descriptor*>(refInterfaceDescriptor->Value());

		// create new Devicehandle object
		Interface *interface = new Interface(libusbDevice, libusbInterfaceDescriptor);
		// initalize handle

		// wrap created Device object to v8
		interface->Wrap(args.This());

#define LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(name) \
		args.This()->Set(V8STR(#name), Integer::New(interface->descriptor->name));
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bLength)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bDescriptorType)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bInterfaceNumber)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bAlternateSetting)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bNumEndpoints)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bInterfaceClass)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bInterfaceSubClass)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bInterfaceProtocol)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(iInterface)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(extra_length)

		return args.This();
	}


	Handle<Value> Interface::IsKernelDriverActiveGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Interface, self, info.Holder())
		
		int isKernelDriverActive = 0;
			
		if ((isKernelDriverActive = libusb_open(self->device, &(self->handle))) >= 0) {
			isKernelDriverActive = libusb_kernel_driver_active(self->handle, self->descriptor->bInterfaceNumber);
		}

		return scope.Close(Integer::New(isKernelDriverActive));
	}

/******************************* Endpoint */
	Persistent<FunctionTemplate> Endpoint::constructor_template;

	Endpoint::Endpoint(libusb_device* _device, libusb_endpoint_descriptor* _endpoint_descriptor) {
		DEBUG("Assigning libusb_device and libusb_endpoint_descriptor structure to self")
		device = _device;
		descriptor = _endpoint_descriptor;
		
		// if bit[7] of endpoint address is set => ENDPOINT_IN (device to host), else: ENDPOINT_OUT (host to device)
		endpoint_type = (descriptor->bEndpointAddress & (1 << 7)) ? (LIBUSB_ENDPOINT_IN) : (LIBUSB_ENDPOINT_OUT);
		// bit[0] and bit[1] of bmAttributes masks transfer_type; 3 = 0000 0011
		transfer_type = (3 & descriptor->bmAttributes);
	}

	Endpoint::~Endpoint() {
		// TODO Close
		DEBUG("Endpoint object destroyed")
	}


	void Endpoint::Initalize(Handle<Object> target) {
		DEBUG("Entering...")
		HandleScope  scope;
		Local<FunctionTemplate> t = FunctionTemplate::New(Endpoint::New);

		// Constructor
		t->InstanceTemplate()->SetInternalFieldCount(1);
		t->SetClassName(String::NewSymbol("Endpoint"));
		Endpoint::constructor_template = Persistent<FunctionTemplate>::New(t);

		Local<ObjectTemplate> instance_template = t->InstanceTemplate();

		// Constants
		// no constants at the moment
	
		// Properties
		instance_template->SetAccessor(V8STR("__endpointType"), Endpoint::EndpointTypeGetter);
		instance_template->SetAccessor(V8STR("__transferType"), Endpoint::TransferTypeGetter);

		// methods exposed to node.js

		// Make it visible in JavaScript
		target->Set(String::NewSymbol("Endpoint"), t->GetFunction());	
		DEBUG("Leave")
	}

	Handle<Value> Endpoint::New(const Arguments& args) {
		HandleScope scope;
		DEBUG("New Endpoint object created")

		// need libusb_device structure as first argument
		if (args.Length() != 2 || !args[0]->IsExternal() || !args[1]->IsExternal()) {
			THROW_BAD_ARGS("Device::New argument is invalid. [object:external:libusb_device, object:external:libusb_external_descriptor]!") 
		}

		// make local value reference to first parameter
		Local<External> refDevice = Local<External>::Cast(args[0]);
		Local<External> refEndpointDescriptor = Local<External>::Cast(args[1]);

		// cast local reference to local 
		libusb_device *libusbDevice = static_cast<libusb_device*>(refDevice->Value());
		libusb_endpoint_descriptor *libusbEndpointDescriptor = static_cast<libusb_endpoint_descriptor*>(refEndpointDescriptor->Value());

		// create new Endpoint object
		Endpoint *endpoint = new Endpoint(libusbDevice, libusbEndpointDescriptor);
		// initalize handle

		// wrap created Endpoint object to v8
		endpoint->Wrap(args.This());

#define LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(name) \
		args.This()->Set(V8STR(#name), Integer::New(endpoint->descriptor->name));
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bLength)
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bDescriptorType)
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bEndpointAddress)
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bmAttributes)
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(wMaxPacketSize)
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bInterval)
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bRefresh)
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bSynchAddress)
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(extra_length)

		return args.This();
	}

	Handle<Value> Endpoint::EndpointTypeGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Endpoint, self, info.Holder())
		
		return scope.Close(Integer::New(self->endpoint_type));
	}

	Handle<Value> Endpoint::TransferTypeGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Endpoint, self, info.Holder())
		
		return scope.Close(Integer::New(self->transfer_type));
	}

	void Callback::DispatchAsynchronousUsbTransfer(libusb_transfer *transfer) {
		
	}

#define TRANSFER_ARGUMENTS_LEFT _transfer, handle 
#define TRANSFER_ARGUMENTS_MIDDLE _buffer, buflen
#define TRANSFER_ARGUMENTS_RIGHT Callback::DispatchAsynchronousUsbTransfer, _user_data, _timeout
#define TRANSFER_ARGUMENTS_DEFAULT TRANSFER_ARGUMENTS_LEFT, device->bEndpointAddress, TRANSFER_ARGUMENTS_MIDDLE, TRANSFER_ARGUMENTS_RIGHT
	int Endpoint::FillTransferStructure(libusb_transfer *_transfer, unsigned char *_buffer, void *_user_data, uint32_t _timeout, unsigned int num_iso_packets) {
		unsigned int buflen = -1; // TODO
		int err = 0;


		switch (transfer_type) {
			case LIBUSB_TRANSFER_TYPE_BULK:
				libusb_fill_bulk_transfer(TRANSFER_ARGUMENTS_DEFAULT);
				break;
			case LIBUSB_TRANSFER_TYPE_INTERRUPT:
				libusb_fill_interrupt_transfer(TRANSFER_ARGUMENTS_DEFAULT);
				break;
			case LIBUSB_TRANSFER_TYPE_CONTROL:
				libusb_fill_control_transfer(TRANSFER_ARGUMENTS_LEFT, _buffer, TRANSFER_ARGUMENTS_RIGHT);
				break;
			case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
				libusb_fill_iso_transfer(TRANSFER_ARGUMENTS_LEFT, device->bEndpointAddress, TRANSFER_ARGUMENTS_MIDDLE, num_iso_packets, TRANSFER_ARGUMENTS_RIGHT);
				break;
			default:
				err = -1;
		}

		return err;
	}

	/**
	 * @param array byte-array
	 * @param function js-callback[status]
	 * @param int (optional) timeout timeout in milliseconds
	 */
	Handle<Value> Endpoint::Write(const Arguments& args) {
		LOCAL(Endpoint, self, args.This())
		return scope.Close(True());

		uint32_t timeout = 0;

		// need libusb_device structure as first argument
		if (args.Length() < 2 || !args[0]->IsArray() || !args[1]->IsFunction()) {
			THROW_BAD_ARGS("Endpoint::Write expects at least 2 arguments [array[] data, function:callback [, uint:timeout_in_ms, uint:transfer_flags]]!") 
		}

		if (args.Length() >= 3) {
			if (!args[2]->IsUint32()) {			
				THROW_BAD_ARGS("Endpoint::Write expects unsigned int as timeout parameter")
			} else {
				timeout = args[2]->Uint32Value();
			}
		}
		
		// TODO Isochronous transfer mode 
		libusb_transfer* transfer = libusb_alloc_transfer(0);

		if (self->FillTransferStructure(transfer, NULL, NULL, timeout, 0) < 0) {
			THROW_BAD_ARGS("Could not fill USB packet structure on device!")
		}
	}
}