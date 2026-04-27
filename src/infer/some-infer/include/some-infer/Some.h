#ifndef SOME_H
#define SOME_H

#include <filesystem>
#include <functional>

#include <audio-util/SndfileVio.h>
#include <some-infer/Provider.h>
#include <some-infer/SomeGlobal.h>

namespace Some
{
    class SomeModel;

    struct SOME_INFER_EXPORT Midi {
        int note, start, duration;
    };

    struct SOME_INFER_EXPORT Dur {
        int note, start, duration;
    };

    class SOME_INFER_EXPORT Some {
    public:
        explicit Some(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id);
        ~Some();

        bool is_open() const;

        bool get_midi(const std::filesystem::path &filepath, std::vector<Midi> &midis, float tempo, std::string &msg,
                      const std::function<void(int)> &progressChanged) const;

        void terminate() const;

    private:
        std::unique_ptr<SomeModel> m_some;
    };
} // namespace Some

#endif // SOME_H
