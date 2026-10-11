"""Review-guarded Windows changes to the pinned UxPlay backend; no saved media."""
from pathlib import Path
import sys
root = Path(sys.argv[1])
def patch(relative, changes):
    path = root / relative
    source = path.read_text()
    for anchor, replacement, expected in changes:
        if source.count(anchor) != expected:
            raise SystemExit(f"Pinned UxPlay context changed: {relative}: {anchor!r}")
        source = source.replace(anchor, replacement)
    path.write_text(source)
patch('uxplay.cpp', [
    ('#include <stddef.h>\n', '#include <stddef.h>\n#include <atomic>\n', 1),
    ('static gboolean feedback_callback(gpointer loop) {\n',
     'static std::atomic<bool> kagami_mirroring(false);\n'
     'extern "C" void kagami_disconnected() {\n'
     '    if (kagami_mirroring.exchange(false)) {\n'
     '        fputs("\\nKAGAMI_AIRPLAY_DISCONNECTED\\n", stdout); fflush(stdout);\n'
     '    }\n}\n'
     'static gboolean feedback_callback(gpointer loop) {\n', 1),
    ('        if (missed_feedback_limit && missed_feedback > missed_feedback_limit) {\n',
     '        if (missed_feedback_limit && missed_feedback > missed_feedback_limit) {\n            kagami_disconnected();\n', 1),
    ('    case RESET_TYPE_RTP_SHUTDOWN:\n', '    case RESET_TYPE_RTP_SHUTDOWN:\n        kagami_disconnected();\n', 1),
    ('    if (open_connections == 0) {\n', '    if (open_connections == 0) {\n        kagami_disconnected();\n', 1),
    ('extern "C" void conn_reset (void *cls, int reason) {\n', 'extern "C" void conn_reset (void *cls, int reason) {\n    kagami_disconnected();\n', 1),
    ('extern "C" void video_process (void *cls, raop_ntp_t *ntp, video_decode_struct *data) {\n',
     'extern "C" void video_process (void *cls, raop_ntp_t *ntp, video_decode_struct *data) {\n'
     '    if (!kagami_mirroring.exchange(true)) { fputs("\\nKAGAMI_AIRPLAY_VIDEO\\n", stdout); fflush(stdout); }\n', 1),
    ('    LOGI("UxPlay %s: An Open-Source AirPlay mirroring and audio-streaming server.", VERSION);\n',
     '    fputs("KAGAMI_AIRPLAY_BUILD_V1\\n", stdout); fflush(stdout);\n'
     '    LOGI("UxPlay %s: An Open-Source AirPlay mirroring and audio-streaming server.", VERSION);\n', 1),
])
patch('lib/raop_rtp_mirror.c', [
    ('#include "raop.h"\n', '#include "raop.h"\nextern void kagami_disconnected(void);\n', 1),
    ('            if (payload == NULL && ret == 0) {\n', '            if (payload == NULL && ret == 0) {\n                kagami_disconnected();\n', 1),
    ('            if (ret == 0) {\n', '            if (ret == 0) {\n                kagami_disconnected();\n', 1),
])
patch('renderers/audio_renderer.c', [
    ('#include <math.h>\n', '#include <math.h>\n#include <stdio.h>\n', 1),
    ('"stream-type", 0, "is-live", TRUE, "format", GST_FORMAT_TIME, NULL);',
     '"stream-type", 0, "is-live", TRUE, "format", GST_FORMAT_TIME, "max-buffers", (guint64) 64, "max-bytes", (guint64) 0, "max-time", (guint64) (2 * GST_SECOND), "leaky-type", 2, NULL);', 1),
    ('        logger_log(logger, LOGGER_INFO, "GStreamer error (audio): %s %s", GST_MESSAGE_SRC_NAME(message),err->message);',
     '        fputs("\\nKAGAMI_AIRPLAY_AUDIO_FAILED\\n", stdout); fflush(stdout);\n'
     '        logger_log(logger, LOGGER_INFO, "GStreamer error (audio): %s %s", GST_MESSAGE_SRC_NAME(message),err->message);', 1),
])
patch('renderers/video_renderer.c', [
    ('"stream-type", 0, "is-live", TRUE, "format", GST_FORMAT_TIME, NULL);',
     '"stream-type", 0, "is-live", TRUE, "format", GST_FORMAT_TIME, "max-buffers", (guint64) 64, "max-bytes", (guint64) (8 * 1024 * 1024), "max-time", (guint64) GST_SECOND, "leaky-type", 2, NULL);', 1),
])
