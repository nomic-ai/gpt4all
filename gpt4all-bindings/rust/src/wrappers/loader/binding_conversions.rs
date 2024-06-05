use crate::bindings::llmodel_gpu_device;
use crate::wrappers::ffi_utils::string_from_ptr;
use crate::wrappers::loader::domain::GpuDevice;

impl From<llmodel_gpu_device> for GpuDevice {
    fn from(value: llmodel_gpu_device) -> Self {
        Self {
            index: value.index,
            device_type: value.type_,
            heap_size: value.heapSize,
            name: string_from_ptr(value.name).unwrap_or_default(),
            vendor: string_from_ptr(value.vendor).unwrap_or_default(),
        }
    }
}
