python
import gdb
from pathlib import Path
inferior = gdb.selected_inferior()
ram = None
for thread in inferior.threads():
    thread.switch()
    frame = gdb.newest_frame()
    while frame is not None:
        try:
            candidate = int(frame.read_var('rdram'))
            if candidate:
                ram = candidate
                break
        except Exception:
            pass
        frame = frame.older()
    if ram:
        break
print('CLAY_THREAD_SNAPSHOT RDRAM', hex(ram or 0))
print('running head', gdb.parse_and_eval('running_queue_impl'))
if ram:
    dump = Path('analysis/debug-rdram.bin')
    dump.write_bytes(bytes(inferior.read_memory(ram, 0x800000)))
    print('CLAY_RDRAM_SNAPSHOT', str(dump), 'bytes', dump.stat().st_size)
    for thread in inferior.threads():
        if not (thread.name or '').startswith('Game Thread'):
            continue
        thread.switch()
        try:
            address = int(gdb.parse_and_eval('thread_self')) & 0xffffffff
            obj = gdb.parse_and_eval('(OSThread*)' + str(ram + (address & 0x7fffff))).dereference()
            print(thread.name, hex(address), 'id', obj['id'], 'pri', obj['priority'],
                  'state', obj['state'], 'queue', hex(int(obj['queue']) & 0xffffffff),
                  'next', hex(int(obj['next']) & 0xffffffff))
        except Exception as error:
            print(thread.name, error)
        frame = gdb.newest_frame()
        while frame is not None:
            try:
                context = frame.read_var('ctx').dereference()
                print('CLAY_GUEST_CONTEXT', thread.name, frame.name(),
                      {name: hex(int(context[name]) & 0xffffffffffffffff)
                       for name in ('r2', 'r4', 'r5', 'r6', 'r7', 'r8', 'r9',
                                    'r16', 'r17', 'r18', 'r19', 'r29', 'r31')})
                break
            except Exception:
                pass
            frame = frame.older()
end
