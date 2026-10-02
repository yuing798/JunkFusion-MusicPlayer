#include "LinuxSystemAudioControl.hpp"

#include <dbus/dbus.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

static constexpr const char* kMprisServiceName{"org.mpris.MediaPlayer2.junkfusion"};
static constexpr const char* kMprisObjectPath{"/org/mpris/MediaPlayer2"};
static constexpr const char* kMprisRootInterface{"org.mpris.MediaPlayer2"};
static constexpr const char* kMprisPlayerInterface{"org.mpris.MediaPlayer2.Player"};
static constexpr const char* kMprisPropertiesInterface{"org.freedesktop.DBus.Properties"};
static constexpr const char* kMprisIntrospectableInterface{"org.freedesktop.DBus.Introspectable"};

// 内省 XML，供 DBus 客户端（如 KDE Connect / GNOME 控制中心）发现接口
static constexpr const char* kIntrospectionXml{
    "<node>"
    "  <interface name='org.freedesktop.DBus.Introspectable'>"
    "    <method name='Introspect'><arg name='xml' type='s' direction='out'/></method>"
    "  </interface>"
    "  <interface name='org.freedesktop.DBus.Properties'>"
    "    <method name='Get'><arg name='interface_name' type='s' direction='in'/>"
    "    <arg name='property_name' type='s' direction='in'/><arg name='value' type='v' "
    "direction='out'/></method>"
    "    <method name='GetAll'><arg name='interface_name' type='s' direction='in'/>"
    "    <arg name='properties' type='a{sv}' direction='out'/></method>"
    "  </interface>"
    "  <interface name='org.mpris.MediaPlayer2'>"
    "    <method name='Raise'/><method name='Quit'/>"
    "    <property name='CanQuit' type='b' access='read'/>"
    "    <property name='Identity' type='s' access='read'/>"
    "  </interface>"
    "  <interface name='org.mpris.MediaPlayer2.Player'>"
    "    <method name='Next'/><method name='Previous'/><method name='Pause'/>"
    "    <method name='PlayPause'/><method name='Stop'/><method name='Play'/>"
    "    <method name='Seek'><arg name='Offset' type='x' direction='in'/></method>"
    "    <method name='SetPosition'><arg name='TrackId' type='o' direction='in'/>"
    "    <arg name='Position' type='x' direction='in'/></method>"
    "    <property name='PlaybackStatus' type='s' access='read'/>"
    "    <property name='Metadata' type='a{sv}' access='read'/>"
    "    <property name='CanGoNext' type='b' access='read'/>"
    "    <property name='CanGoPrevious' type='b' access='read'/>"
    "    <property name='CanPlay' type='b' access='read'/>"
    "    <property name='CanPause' type='b' access='read'/>"
    "    <property name='CanSeek' type='b' access='read'/>"
    "  </interface>"
    "</node>"
};

// 平台实现细节全部收敛进 Impl
struct LinuxSystemAudioControl::Impl {
    DBusConnection* connection{nullptr};
    DBusError error{};

    std::thread readerThread;
    std::atomic<bool> running{false};
    std::atomic<bool> initialized{false};

    // 当前对外广播的状态（供 Properties.Get / GetAll 读取）
    std::string playbackStatus{"Stopped"};
    std::string currentTitle;
    std::vector<std::string> currentArtists;

    // 供 PlayPause 判断方向使用
    std::atomic<PlaybackState> currentPlaybackState{PlaybackState::Pause};
};

// ---------------------------------------------------------------------------
// 工具：发送 PropertiesChanged 信号（含单个 string 属性变化）
// ---------------------------------------------------------------------------
static void
emitStringPropertyChanged(DBusConnection* connection, const char* property, const char* value) {
    DBusMessage* signal =
        dbus_message_new_signal(kMprisObjectPath, kMprisPropertiesInterface, "PropertiesChanged");
    if (!signal) {
        return;
    }

    const char* interfaceName = kMprisPlayerInterface;
    DBusMessageIter args{};
    DBusMessageIter dict{};
    DBusMessageIter entry{};
    DBusMessageIter variant{};

    dbus_message_append_args(signal, DBUS_TYPE_STRING, &interfaceName, DBUS_TYPE_INVALID);
    dbus_message_iter_init_append(signal, &args);
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &dict);
    dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &property);
    dbus_message_iter_open_container(
        &entry,
        DBUS_TYPE_VARIANT,
        DBUS_TYPE_STRING_AS_STRING,
        &variant
    );
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &value);
    dbus_message_iter_close_container(&entry, &variant);
    dbus_message_iter_close_container(&dict, &entry);
    dbus_message_iter_close_container(&args, &dict);

    dbus_connection_send(connection, signal, nullptr);
    dbus_message_unref(signal);
}

// ---------------------------------------------------------------------------
// 工具：向属性容器里追加一个 {sv} 条目（供 GetAll 使用）
// ---------------------------------------------------------------------------
static void appendStringEntry(DBusMessageIter* dict, const char* key, const char* value) {
    DBusMessageIter entry{};
    DBusMessageIter variant{};
    dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
    dbus_message_iter_open_container(
        &entry,
        DBUS_TYPE_VARIANT,
        DBUS_TYPE_STRING_AS_STRING,
        &variant
    );
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &value);
    dbus_message_iter_close_container(&entry, &variant);
    dbus_message_iter_close_container(dict, &entry);
}

static void appendBoolEntry(DBusMessageIter* dict, const char* key, dbus_bool_t value) {
    DBusMessageIter entry{};
    DBusMessageIter variant{};
    dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
    dbus_message_iter_open_container(
        &entry,
        DBUS_TYPE_VARIANT,
        DBUS_TYPE_BOOLEAN_AS_STRING,
        &variant
    );
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &value);
    dbus_message_iter_close_container(&entry, &variant);
    dbus_message_iter_close_container(dict, &entry);
}

// ---------------------------------------------------------------------------
// 工具：回复单一属性的 Variant（供 Properties.Get 使用）
// ---------------------------------------------------------------------------
static bool replyStringVariant(DBusConnection* connection, DBusMessage* msg, const char* value) {
    DBusMessage* reply = dbus_message_new_method_return(msg);
    if (!reply) return false;
    DBusMessageIter args{}, variant{};
    dbus_message_iter_init_append(reply, &args);
    dbus_message_iter_open_container(
        &args,
        DBUS_TYPE_VARIANT,
        DBUS_TYPE_STRING_AS_STRING,
        &variant
    );
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &value);
    dbus_message_iter_close_container(&args, &variant);
    dbus_connection_send(connection, reply, nullptr);
    dbus_message_unref(reply);
    return true;
}

static bool replyBoolVariant(DBusConnection* connection, DBusMessage* msg, dbus_bool_t value) {
    DBusMessage* reply = dbus_message_new_method_return(msg);
    if (!reply) return false;
    DBusMessageIter args{}, variant{};
    dbus_message_iter_init_append(reply, &args);
    dbus_message_iter_open_container(
        &args,
        DBUS_TYPE_VARIANT,
        DBUS_TYPE_BOOLEAN_AS_STRING,
        &variant
    );
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &value);
    dbus_message_iter_close_container(&args, &variant);
    dbus_connection_send(connection, reply, nullptr);
    dbus_message_unref(reply);
    return true;
}

// ---------------------------------------------------------------------------
// 工具：生成空方法返回
// ---------------------------------------------------------------------------
static bool replyEmpty(DBusConnection* connection, DBusMessage* msg) {
    DBusMessage* reply = dbus_message_new_method_return(msg);
    if (!reply) {
        return false;
    }
    dbus_bool_t ok = dbus_connection_send(connection, reply, nullptr);
    dbus_message_unref(reply);
    return ok;
}

// ---------------------------------------------------------------------------
// D-Bus 消息分发（注册对象路径后，所有对本服务的调用都进这里）
// ---------------------------------------------------------------------------
DBusHandlerResult LinuxSystemAudioControl::handleMessage(
    DBusConnection* connection,
    DBusMessage* msg,
    void* userData
) {
    auto* self = static_cast<LinuxSystemAudioControl*>(userData);
    const char* interface = dbus_message_get_interface(msg);
    const char* member = dbus_message_get_member(msg);
    const std::string memberStr{member ? member : ""};

    // ---- Introspectable ----
    if (interface && std::string(interface) == kMprisIntrospectableInterface &&
        memberStr == "Introspect") {
        DBusMessage* reply = dbus_message_new_method_return(msg);
        if (reply) {
            const char* xml = kIntrospectionXml;
            dbus_message_append_args(reply, DBUS_TYPE_STRING, &xml, DBUS_TYPE_INVALID);
            dbus_connection_send(connection, reply, nullptr);
            dbus_message_unref(reply);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    // ---- Properties.Get ----
    if (interface && std::string(interface) == kMprisPropertiesInterface && memberStr == "Get") {
        const char* propName = nullptr;
        if (dbus_message_get_args(msg, nullptr, DBUS_TYPE_STRING, &propName, DBUS_TYPE_INVALID) &&
            propName) {
            std::string name{propName};
            const auto* impl = self->mImpl.get();

            if (name == "PlaybackStatus") {
                return replyStringVariant(connection, msg, impl->playbackStatus.c_str())
                           ? DBUS_HANDLER_RESULT_HANDLED
                           : DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            }
            if (name == "Identity") {
                return replyStringVariant(connection, msg, "JunkFusion MusicPlayer")
                           ? DBUS_HANDLER_RESULT_HANDLED
                           : DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            }
            if (name == "CanPlay" || name == "CanPause" || name == "CanGoNext" ||
                name == "CanGoPrevious" || name == "CanSeek") {
                return replyBoolVariant(connection, msg, TRUE)
                           ? DBUS_HANDLER_RESULT_HANDLED
                           : DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            }
        }
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    // ---- Properties.GetAll ----
    if (interface && std::string(interface) == kMprisPropertiesInterface && memberStr == "GetAll") {
        DBusMessage* reply = dbus_message_new_method_return(msg);
        if (!reply) {
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        const auto* impl = self->mImpl.get();

        DBusMessageIter args{};
        DBusMessageIter dict{};
        dbus_message_iter_init_append(reply, &args);
        dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &dict);

        appendStringEntry(&dict, "PlaybackStatus", impl->playbackStatus.c_str());
        appendStringEntry(&dict, "Identity", "JunkFusion MusicPlayer");
        appendBoolEntry(&dict, "CanGoNext", TRUE);
        appendBoolEntry(&dict, "CanGoPrevious", TRUE);
        appendBoolEntry(&dict, "CanPlay", TRUE);
        appendBoolEntry(&dict, "CanPause", TRUE);
        appendBoolEntry(&dict, "CanSeek", TRUE);

        dbus_message_iter_close_container(&args, &dict);
        dbus_connection_send(connection, reply, nullptr);
        dbus_message_unref(reply);
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    // ---- 媒体控制方法（耳机/键盘媒体键最终会走到这里）----
    if (interface && std::string(interface) == kMprisPlayerInterface) {
        if (memberStr == "Play") {
            if (self->onPlay) {
                self->onPlay();
            }
            replyEmpty(connection, msg);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        if (memberStr == "Pause") {
            if (self->onPause) {
                self->onPause();
            }
            replyEmpty(connection, msg);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        if (memberStr == "PlayPause") {
            // 利用内部记录的状态决定 toggle 方向
            if (self->mImpl->currentPlaybackState.load() == PlaybackState::Play) {
                if (self->onPause) self->onPause();
            } else {
                if (self->onPlay) self->onPlay();
            }
            replyEmpty(connection, msg);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        if (memberStr == "Next") {
            if (self->onNext) {
                self->onNext();
            }
            replyEmpty(connection, msg);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        if (memberStr == "Previous") {
            if (self->onPrevious) {
                self->onPrevious();
            }
            replyEmpty(connection, msg);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        if (memberStr == "Seek") {
            dbus_int64_t offset = 0;
            // 解析 XML 里定义的 'Offset' 参数（类型 'x'，即 int64）
            if (dbus_message_get_args(msg, nullptr, DBUS_TYPE_INT64, &offset, DBUS_TYPE_INVALID)) {
                // MPRIS 规范里 Seek 的偏移量单位是微秒（Microseconds）
                if (offset > 0) {
                    // 正向偏移，代表快进
                    if (self->onFastForward) self->onFastForward();
                } else if (offset < 0) {
                    // 负向偏移，代表快退
                    if (self->onRewind) self->onRewind();
                }
            }
            replyEmpty(connection, msg);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        if (memberStr == "SetPosition") {
            const char* trackId = nullptr;
            dbus_int64_t position = 0;
            // 解析 XML 里定义的 'TrackId' (类型 'o') 和 'Position' (类型 'x')
            if (dbus_message_get_args(
                    msg,
                    nullptr,
                    DBUS_TYPE_OBJECT_PATH,
                    &trackId,
                    DBUS_TYPE_INT64,
                    &position,
                    DBUS_TYPE_INVALID
                )) {
                // MPRIS 的 Position 单位是微秒，转换为秒
                double seconds = static_cast<double>(position) / 1000000.0;
                if (self->onSeek) self->onSeek(seconds);
            }
            replyEmpty(connection, msg);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        if (memberStr == "Stop") {
            // 现代音乐播放器无独立 Stop 状态，降级为 Pause
            if (self->onPause) self->onPause();
            replyEmpty(connection, msg);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void unregisterPath(DBusConnection*, void*) {
    // 释放时的清理回调，此处无额外资源
}

static const DBusObjectPathVTable kObjectVTable{
    unregisterPath,
    LinuxSystemAudioControl::handleMessage,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};

LinuxSystemAudioControl::LinuxSystemAudioControl() : mImpl(std::make_unique<Impl>()) {}

LinuxSystemAudioControl::~LinuxSystemAudioControl() { shutdown(); }

bool LinuxSystemAudioControl::initialize() {
    if (mImpl->initialized.load()) {
        return true;
    }

    dbus_error_init(&mImpl->error);
    mImpl->connection = dbus_bus_get(DBUS_BUS_SESSION, &mImpl->error);
    if (!mImpl->connection) {
        // TODO(连接): 打印 D-Bus 连接失败日志（mImpl->error.message）
        return false;
    }

    // 请求独占 MPRIS 名字；冲突时（已有同名字实例）返回 false
    int ret = dbus_bus_request_name(
        mImpl->connection,
        kMprisServiceName,
        DBUS_NAME_FLAG_DO_NOT_QUEUE,
        &mImpl->error
    );
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
        // TODO(连接): 打印名字占用日志
        return false;
    }

    // 注册对象路径并绑定消息分发（userData 传 this 以便回调）
    if (!dbus_connection_register_object_path(
            mImpl->connection,
            kMprisObjectPath,
            &kObjectVTable,
            this
        )) {
        // TODO(连接): 打印对象路径注册失败日志
        return false;
    }

    mImpl->running.store(true);
    mImpl->readerThread = std::thread([this] {
        while (mImpl->running.load()) {
            dbus_connection_read_write_dispatch(mImpl->connection, 100);
        }
    });

    mImpl->initialized.store(true);
    return true;
}

void LinuxSystemAudioControl::updateMetadata(const MediaMetadata& metadata) {
    if (!mImpl->initialized.load()) {
        return;
    }

    mImpl->currentTitle = metadata.title;
    mImpl->currentArtists = metadata.artists;

    // 广播 Metadata 变化。MPRIS 规范中 Metadata 是 a{sv}，这里先广播 xesam:title。
    // FIXME(连接): 完整的 Metadata 应包含 mpris:trackid(歌曲唯一 id)、mpris:length(时长微秒)、
    //              xesam:artist(字符串数组)、mpris:artUrl(封面地址)，
    //              请接入真实数据后通过一个 emitMetadataChanged() 统一广播。
    emitStringPropertyChanged(mImpl->connection, "xesam:title", metadata.title.c_str());
}

void LinuxSystemAudioControl::updatePlaybackState(PlaybackState state) {
    if (!mImpl->initialized.load()) {
        return;
    }

    // 更新内部状态，供 PlayPause 判断方向
    mImpl->currentPlaybackState.store(state);
    mImpl->playbackStatus = (state == PlaybackState::Play) ? "Playing" : "Paused";

    emitStringPropertyChanged(mImpl->connection, "PlaybackStatus", mImpl->playbackStatus.c_str());
}

void LinuxSystemAudioControl::shutdown() {
    if (!mImpl->initialized.exchange(false)) {
        return;
    }

    mImpl->running.store(false);
    if (mImpl->readerThread.joinable()) {
        mImpl->readerThread.join();
    }

    if (mImpl->connection) {
        dbus_connection_unregister_object_path(mImpl->connection, kMprisObjectPath);
        dbus_connection_flush(mImpl->connection);
        dbus_connection_unref(mImpl->connection);
        mImpl->connection = nullptr;
    }
    dbus_error_free(&mImpl->error);
}