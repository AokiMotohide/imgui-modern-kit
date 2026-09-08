"""Normalize non-executable CodeView object and source filenames in an SDK copy.

COFF offsets/relocations and record lengths are preserved. S_OBJNAME contains a
NUL-terminated filename; the remaining record bytes are zero padding. The
source string-table offsets are retained through fixed-length NUL padding.
The archive consumer is linked/run after normalization, before packaging.
"""
import pathlib, struct, sys
source,destination=map(pathlib.Path,sys.argv[1:3])
data=bytearray(source.read_bytes())
machine,sections=struct.unpack_from('<HH',data)
if machine!=0x8664:raise ValueError('Expected x64 COFF object')
optional=struct.unpack_from('<H',data,16)[0]
changed=0
for index in range(sections):
    header=20+optional+index*40
    name=bytes(data[header:header+8]).rstrip(b'\0')
    size,offset=struct.unpack_from('<II',data,header+16)
    if name!=b'.debug$S' or size<4:continue
    if struct.unpack_from('<I',data,offset)[0]!=4:raise ValueError('Expected CodeView C13')
    cursor=offset+4;end=offset+size
    while cursor+8<=end:
        kind,length=struct.unpack_from('<II',data,cursor);start=cursor+8;stop=start+length
        if stop>end:raise ValueError('Invalid CodeView subsection')
        if kind==0xf3:
            string_start=start
            while string_start<stop:
                nul=data.index(0,string_start,stop)
                original=bytes(data[string_start:nul])
                if len(original)>2 and original[1:3] in (b':\\',b':/'):
                    filename=original.replace(b'\\',b'/').rsplit(b'/',1)[-1]
                    relative=b'toolchain/'+filename
                    if len(relative)>len(original):raise ValueError('Filename normalization must fit')
                    data[string_start:nul]=relative+b'\0'*(len(original)-len(relative))
                string_start=nul+1
        if kind==0xf1:
            record=start
            while record+4<=stop:
                record_length,record_kind=struct.unpack_from('<HH',data,record)
                record_end=record+2+record_length
                if record_end>stop or record_length<2:raise ValueError('Invalid symbol record')
                if record_kind==0x1101:
                    string_start=record+8
                    nul=data.index(0,string_start,record_end)
                    relative=('imkit/objects/'+source.name).encode('utf-8')
                    if len(relative)>nul-string_start:raise ValueError('Replacement must fit existing record')
                    data[string_start:nul]=relative+b'\0'*(nul-string_start-len(relative))
                    changed+=1
                record=record_end
        cursor=offset+((stop-offset+3)&~3)
if not changed:raise ValueError('No object-name record found')
destination.parent.mkdir(parents=True,exist_ok=True)
destination.write_bytes(data)
