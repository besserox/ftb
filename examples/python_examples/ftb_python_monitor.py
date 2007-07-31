from ctypes import *
from time   import sleep

libftb = cdll.LoadLibrary("libftb_polling.so")

class event(Structure):
    _fields_ = [('event_id', c_uint32),
                ('name', c_char * 128),
                ('severity', c_uint32)]

class event_inst(Structure):
    _fields_ = [('event_id', c_uint32),
                ('name', c_char * 128),
                ('severity', c_uint32),
                ('src_namespace', c_uint32),
                ('src_id', c_uint64),
                ('immediate_data', c_char * 512)]

class event_mask(Structure):
    _fields_ = [('event_id', c_uint32),
                ('severity', c_uint32),
                ('src_namespace', c_uint32),
                ('src_id', c_uint64)]

class component_properties(Structure):
    _fields_ = [('com_namespace', c_uint32),
                ('id',c_uint64),
                ('name', c_char * 128),
                ('polling_only', c_int),
                ('polling_queue_len', c_int)]

if __name__ == '__main__':
    properties = component_properties(0x02, 0x200000100, 'PYTHON_MONITOR', 1, 40);
    libftb.FTB_Init(byref(properties))
    mask = event_mask(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffffffffffff)
    libftb.FTB_Reg_catch_polling_all()
    evt = event_inst()
    while(1):
      sleep(10)
      ret = libftb.FTB_Catch(byref(evt))
      while (ret == 0):
        print 'caught event id %d, name %s' % (evt.event_id, evt.name)
        ret = libftb.FTB_Catch(byref(evt))
    libftb.FTB_Finalize()
