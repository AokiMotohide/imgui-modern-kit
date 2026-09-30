#include <imkit/imkit.h>
void (*volatile update)(std::span<const imkit::ToastView>,imkit::ToastViewportState&,double,std::size_t,imkit::ToastEventBuffer&,std::span<const imkit::StableId>)=&imkit::UpdateToastViewport;
void (*volatile draw)(const char*,std::span<const imkit::ToastView>,imkit::ToastViewportState&,double,imkit::ToastEventBuffer&,imkit::ToastViewportOptions,imkit::ComponentOptions)=&imkit::ToastViewport;
bool (imkit::ToastEventBuffer::*volatile push)(imkit::ToastEvent)=&imkit::ToastEventBuffer::Push;
void (imkit::ToastViewportState::*volatile reset)()=&imkit::ToastViewportState::Reset;
int main(){return 0;}
