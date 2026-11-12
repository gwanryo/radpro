import ctypes as C

_ft = C.CDLL('/opt/homebrew/opt/freetype/lib/libfreetype.6.dylib')

FT_LOAD_RENDER = 4


class _Generic(C.Structure):
    _fields_ = [('data', C.c_void_p), ('finalizer', C.c_void_p)]


class _BBox(C.Structure):
    _fields_ = [('xMin', C.c_long), ('yMin', C.c_long), ('xMax', C.c_long), ('yMax', C.c_long)]


class _Bitmap(C.Structure):
    _fields_ = [('rows', C.c_uint), ('width', C.c_uint), ('pitch', C.c_int),
                ('buffer', C.POINTER(C.c_ubyte)), ('num_grays', C.c_ushort),
                ('pixel_mode', C.c_ubyte), ('palette_mode', C.c_ubyte), ('palette', C.c_void_p)]


class _GlyphMetrics(C.Structure):
    _fields_ = [(n, C.c_long) for n in ('width', 'height', 'horiBearingX', 'horiBearingY',
                                        'horiAdvance', 'vertBearingX', 'vertBearingY', 'vertAdvance')]


class _Vector(C.Structure):
    _fields_ = [('x', C.c_long), ('y', C.c_long)]


class _GlyphSlot(C.Structure):
    _fields_ = [('library', C.c_void_p), ('face', C.c_void_p), ('next', C.c_void_p),
                ('glyph_index', C.c_uint), ('generic', _Generic), ('metrics', _GlyphMetrics),
                ('linearHoriAdvance', C.c_long), ('linearVertAdvance', C.c_long),
                ('advance', _Vector), ('format', C.c_int), ('bitmap', _Bitmap),
                ('bitmap_left', C.c_int), ('bitmap_top', C.c_int)]


class _SizeMetrics(C.Structure):
    _fields_ = [('x_ppem', C.c_ushort), ('y_ppem', C.c_ushort), ('x_scale', C.c_long),
                ('y_scale', C.c_long), ('ascender', C.c_long), ('descender', C.c_long),
                ('height', C.c_long), ('max_advance', C.c_long)]


class _Size(C.Structure):
    _fields_ = [('face', C.c_void_p), ('generic', _Generic), ('metrics', _SizeMetrics),
                ('internal', C.c_void_p)]


class _Face(C.Structure):
    _fields_ = [('num_faces', C.c_long), ('face_index', C.c_long), ('face_flags', C.c_long),
                ('style_flags', C.c_long), ('num_glyphs', C.c_long), ('family_name', C.c_char_p),
                ('style_name', C.c_char_p), ('num_fixed_sizes', C.c_int),
                ('available_sizes', C.c_void_p), ('num_charmaps', C.c_int),
                ('charmaps', C.c_void_p), ('generic', _Generic), ('bbox', _BBox),
                ('units_per_EM', C.c_ushort), ('ascender', C.c_short), ('descender', C.c_short),
                ('height', C.c_short), ('max_advance_width', C.c_short),
                ('max_advance_height', C.c_short), ('underline_position', C.c_short),
                ('underline_thickness', C.c_short), ('glyph', C.POINTER(_GlyphSlot)),
                ('size', C.POINTER(_Size)), ('charmap', C.c_void_p)]


class _SfntName(C.Structure):
    _fields_ = [('platform_id', C.c_ushort), ('encoding_id', C.c_ushort),
                ('language_id', C.c_ushort), ('name_id', C.c_ushort),
                ('string', C.POINTER(C.c_ubyte)), ('string_len', C.c_uint)]


_FaceP = C.POINTER(_Face)
_ft.FT_Init_FreeType.argtypes = [C.POINTER(C.c_void_p)]
_ft.FT_New_Face.argtypes = [C.c_void_p, C.c_char_p, C.c_long, C.POINTER(_FaceP)]
_ft.FT_Set_Char_Size.argtypes = [_FaceP, C.c_long, C.c_long, C.c_uint, C.c_uint]
_ft.FT_Get_Sfnt_Name_Count.argtypes = [_FaceP]
_ft.FT_Get_Sfnt_Name_Count.restype = C.c_uint
_ft.FT_Get_Sfnt_Name.argtypes = [_FaceP, C.c_uint, C.POINTER(_SfntName)]
_ft.FT_Get_First_Char.argtypes = [_FaceP, C.POINTER(C.c_uint)]
_ft.FT_Get_First_Char.restype = C.c_ulong
_ft.FT_Get_Next_Char.argtypes = [_FaceP, C.c_ulong, C.POINTER(C.c_uint)]
_ft.FT_Get_Next_Char.restype = C.c_ulong
_ft.FT_Load_Glyph.argtypes = [_FaceP, C.c_uint, C.c_int]

_library = C.c_void_p()
if _ft.FT_Init_FreeType(C.byref(_library)):
    raise RuntimeError('FT_Init_FreeType failed')


def version():
    a, b, c = C.c_int(), C.c_int(), C.c_int()
    _ft.FT_Library_Version(_library, C.byref(a), C.byref(b), C.byref(c))
    return (a.value, b.value, c.value)


class _Name:
    pass


class _BitmapView:
    def __init__(self, bitmap):
        self.rows = bitmap.rows
        self.width = bitmap.width
        self.pitch = bitmap.pitch
        self.buffer = [bitmap.buffer[i] for i in range(bitmap.rows * abs(bitmap.pitch))]


class _GlyphView:
    def __init__(self, slot):
        self.bitmap_left = slot.bitmap_left
        self.bitmap_top = slot.bitmap_top
        self.advance = slot.advance
        self.bitmap = _BitmapView(slot.bitmap)


class Face:
    def __init__(self, path):
        self._face = _FaceP()
        if _ft.FT_New_Face(_library, path.encode('utf-8'), 0, C.byref(self._face)):
            raise RuntimeError('FT_New_Face failed: ' + path)
        self.glyph = None

    @property
    def is_scalable(self):
        return bool(self._face.contents.face_flags & 1)

    def set_char_size(self, width=0, height=0, hres=72, vres=72):
        if _ft.FT_Set_Char_Size(self._face, width, height, hres, vres):
            raise RuntimeError('FT_Set_Char_Size failed')

    @property
    def sfnt_name_count(self):
        return _ft.FT_Get_Sfnt_Name_Count(self._face)

    def get_sfnt_name(self, index):
        raw = _SfntName()
        if _ft.FT_Get_Sfnt_Name(self._face, index, C.byref(raw)):
            raise RuntimeError('FT_Get_Sfnt_Name failed')
        name = _Name()
        name.platform_id = raw.platform_id
        name.encoding_id = raw.encoding_id
        name.language_id = raw.language_id
        name.name_id = raw.name_id
        name.string = bytes(raw.string[i] for i in range(raw.string_len))
        return name

    def get_chars(self):
        gindex = C.c_uint()
        charcode = _ft.FT_Get_First_Char(self._face, C.byref(gindex))
        while gindex.value != 0:
            yield charcode, gindex.value
            charcode = _ft.FT_Get_Next_Char(self._face, charcode, C.byref(gindex))

    def load_glyph(self, index, flags):
        if _ft.FT_Load_Glyph(self._face, index, flags):
            raise RuntimeError('FT_Load_Glyph failed')
        self.glyph = _GlyphView(self._face.contents.glyph.contents)

    @property
    def size(self):
        return self._face.contents.size.contents.metrics
