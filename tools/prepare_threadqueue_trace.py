#!/usr/bin/env python3
"""Repair linked-list removal and optionally trace an immutable SDK queue copy; ClayTality 64 by djultra64."""
import argparse
import hashlib
from pathlib import Path

EXPECTED = 'b9bb55edeb9cb99a153d00d80a13c5a09140113403499af7656fd9489d38f305'
SITE = '    PTR(OSThread)* cur = queue_to_ptr(PASS_RDRAM queue_);\n    OSThread* toadd = TO_PTR(OSThread, toadd_);'
REMOVAL_BEGIN = 'bool ultramodern::thread_queue_remove(RDRAM_ARG PTR(PTR(OSThread)) queue_, PTR(OSThread) t_) {'
REMOVAL_END = 'bool ultramodern::thread_queue_empty(RDRAM_ARG PTR(PTR(OSThread)) queue_) {'
REMOVAL = r'''bool ultramodern::thread_queue_remove(RDRAM_ARG PTR(PTR(OSThread)) queue_, PTR(OSThread) t_) {
    PTR(OSThread)* link = queue_to_ptr(PASS_RDRAM queue_);
    while (*link != NULLPTR) {
        OSThread* current = TO_PTR(OSThread, *link);
        if (*link == t_) {
            *link = current->next;
            return true;
        }
        link = &current->next;
    }
    return false;
}

'''
TRACE = r'''    // Optional diagnosis before mutating intrusive links.
    if (std::getenv("CLAY_TRACE_QUEUES")) {
    PTR(OSThread) trace_self = ultramodern::this_thread();
    int trace_id = trace_self ? TO_PTR(OSThread, trace_self)->id : 0;
    OSThread* trace_target = TO_PTR(OSThread, toadd_);
    fprintf(stderr,"{\"event\":\"queue_insert\",\"queue\":\"%08X\",\"target\":\"%08X\",\"id\":%d,\"self\":%d,\"old_queue\":\"%08X\",\"old_next\":\"%08X\"}\n",uint32_t(queue_),uint32_t(toadd_),trace_target->id,trace_id,uint32_t(trace_target->queue),uint32_t(trace_target->next));
    PTR(OSThread) trace_node = *queue_to_ptr(PASS_RDRAM queue_);
    unsigned trace_depth = 0;
    while (trace_node != NULLPTR) {
        if (trace_node == toadd_) {
            fprintf(stderr,"{\"event\":\"duplicate_thread_insert\",\"queue\":\"%08X\",\"target\":\"%08X\",\"id\":%d,\"self\":%d,\"depth\":%u}\n",uint32_t(queue_),uint32_t(toadd_),trace_target->id,trace_id,trace_depth);
            fflush(stderr);
            throw std::runtime_error("duplicate_thread_insert");
        }
        if (++trace_depth > 128) {
            fflush(stderr);
            throw std::runtime_error("existing_thread_queue_cycle");
        }
        auto trace_address=uint32_t(trace_node);
        if ((trace_address & 0x1FFFFFFF) > 0x800000-sizeof(OSThread)) {
            fflush(stderr);
            throw std::runtime_error("invalid_thread_queue_pointer");
        }
        trace_node = TO_PTR(OSThread, trace_node)->next;
    }
    } // CLAY_TRACE_QUEUES
'''

def prepare(source, output):
    source = Path(source) / 'ultramodern/src/threadqueue.cpp'
    raw = source.read_bytes()
    if hashlib.sha256(raw).hexdigest() != EXPECTED:
        raise ValueError('SDK thread queue source fingerprint changed')
    text = raw.decode()
    if text.count(SITE) != 1:
        raise ValueError('SDK insertion instrumentation site is not unique')
    for marker in (REMOVAL_BEGIN, REMOVAL_END):
        if text.count(marker) != 1:
            raise ValueError('SDK removal transform site is not unique')
    begin, end = text.index(REMOVAL_BEGIN), text.index(REMOVAL_END)
    text = text[:begin] + REMOVAL + text[end:]
    text = '#include <cstdio>\n#include <cstdlib>\n#include <stdexcept>\n' + text.replace(SITE, TRACE + SITE)
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    if not output.exists() or output.read_text() != text:
        output.write_text(text)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sdk_runtime', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    prepare(args.sdk_runtime, args.output)
