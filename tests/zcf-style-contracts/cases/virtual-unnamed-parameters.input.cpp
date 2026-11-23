class IConnection : public virtual Base
{
public:
    virtual void SetAudioFormat(AudioFormat) = 0;
    virtual Input::IInputControllerWeakPtr QueryInputController(Input::IInputDelegateWeakPtr) = 0;
    virtual void OnFormats(std::vector<ClipboardFormat>) = 0;
    virtual ~IConnection() = default;
};

#define DECLARE_TRANSCODER(_NAME_) \
    class _NAME_##Transcoder \
    { \
        virtual ~_NAME_##Transcoder() = default; \
    };

void
Use()
{
    AudioFormat format(other);
    Widget widget(AudioFormat);
    GetRef(index) = 0;
}
