with open("/home/hjl/.codon/lib/codon/stdlib/asyncio.codon", "r") as f:
    content = f.read()

# Fix 1: _call_soon_no_duplicate check work_curr
target1 = """    def _call_soon_no_duplicate(self, work: WorkItem):
        with self._lock:
            self._ensure_open()
            node = self._work_head"""
repl1 = """    def _call_soon_no_duplicate(self, work: WorkItem):
        with self._lock:
            self._ensure_open()
            if self._work_curr is not None and self._work_curr.coro == work.coro:
                return False
            node = self._work_head"""

# Fix 2: guard g.__done__() before g.__resume__()
target2 = """        if work is not None:
            g = Generator[None](work.coro)
            exception: Optional[BaseException] = None"""
repl2 = """        if work is not None:
            g = Generator[None](work.coro)
            if g.__done__():
                self._work_curr = None
                return False
            exception: Optional[BaseException] = None"""

assert target1 in content, "target1 not found"
assert target2 in content, "target2 not found"
content = content.replace(target1, repl1, 1).replace(target2, repl2, 1)

with open("/home/hjl/.codon/lib/codon/stdlib/asyncio.codon", "w") as f:
    f.write(content)
print("asyncio.codon patched successfully!")
