"""Small desktop-only libopus wrapper. No codec binary is redistributed."""
import ctypes as C
import ctypes.util
import os


class Opus:
    def __init__(self):
        path = os.environ.get('ZUMI_OPUS_LIBRARY') or ctypes.util.find_library('opus')
        if not path:
            raise RuntimeError('Install libopus or set ZUMI_OPUS_LIBRARY to its local library path')
        self.lib = lib = C.CDLL(path)
        lib.opus_encoder_create.argtypes = [C.c_int, C.c_int, C.c_int, C.POINTER(C.c_int)]
        lib.opus_encoder_create.restype = C.c_void_p
        lib.opus_decoder_create.argtypes = [C.c_int, C.c_int, C.POINTER(C.c_int)]
        lib.opus_decoder_create.restype = C.c_void_p
        lib.opus_encode.argtypes = [C.c_void_p, C.c_void_p, C.c_int, C.c_void_p, C.c_int]
        lib.opus_decode.argtypes = [C.c_void_p, C.c_void_p, C.c_int, C.c_void_p, C.c_int, C.c_int]
        lib.opus_encoder_destroy.argtypes = [C.c_void_p]
        lib.opus_decoder_destroy.argtypes = [C.c_void_p]
        self.encoder = self.decoder = None
        error = C.c_int()
        self.encoder = lib.opus_encoder_create(16000, 1, 2048, C.byref(error))
        if error.value or not self.encoder:
            self.close(); raise ValueError('Opus encoder unavailable')
        # libopus can decode a 24 kHz server stream directly to 16 kHz PCM.
        self.decoder = lib.opus_decoder_create(16000, 1, C.byref(error))
        if error.value or not self.decoder:
            self.close(); raise ValueError('Opus decoder unavailable')

    def encode(self, pcm):
        if len(pcm) != 1920 or not self.encoder:
            raise ValueError('Expected 60 ms of mono PCM16 at 16 kHz')
        out = C.create_string_buffer(4000)
        size = self.lib.opus_encode(self.encoder, C.create_string_buffer(pcm), 960, out, len(out))
        if size < 0:
            raise ValueError('Opus encode failed')
        return out.raw[:size]

    def decode(self, packet):
        if not 0 < len(packet) <= 4000 or not self.decoder:
            raise ValueError('Invalid Opus packet')
        out = C.create_string_buffer(3840)  # At most 120 ms, irrespective of packet duration.
        count = self.lib.opus_decode(self.decoder, C.create_string_buffer(packet), len(packet), out, 1920, 0)
        if count < 0:
            raise ValueError('Opus decode failed')
        return out.raw[:count * 2]

    def close(self):
        if self.encoder:
            self.lib.opus_encoder_destroy(self.encoder); self.encoder = None
        if self.decoder:
            self.lib.opus_decoder_destroy(self.decoder); self.decoder = None
