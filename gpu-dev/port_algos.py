#!/usr/bin/env python3
# Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
# GNU General Public License version 3, see the LICENSE file.
"""
One-shot porting tool: copies the CPU RandomHash2 hash implementations into
cudalib/rh2/algos/ and makes them portable (host C++ and CUDA device code):

  * project includes are dropped (the master header includes everything in order)
  * constant tables become RH2_CONST (device global memory under nvcc)
  * every function definition gets the RH2_DEV qualifier
  * SSE-only snippets are replaced by scalar equivalents

The output is committed as normal source; re-run only when the CPU algos change.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "cudalib", "rh2", "algos")

PARNIAN = os.path.join(ROOT, "MinersLib", "Parnian")
ALGO = os.path.join(ROOT, "MinersLib", "Algo")

# (source path, output name)
FILES = [
    (os.path.join(ALGO, "sph_types.h"), "sph_types.h"),
    (os.path.join(ALGO, "sph_sha0.h"), "sph_sha0.h"),
    (os.path.join(ALGO, "sph_sha1.h"), "sph_sha1.h"),
    (os.path.join(ALGO, "sph_panama.h"), "sph_panama.h"),
    (os.path.join(ALGO, "sph-haval.h"), "sph_haval.h"),
    (os.path.join(ALGO, "sha0.cpp"), "sha0.h"),
    (os.path.join(ALGO, "sha1.cpp"), "sha1.h"),
    (os.path.join(ALGO, "sph_panama.cpp"), "panama.h"),
    (os.path.join(ALGO, "haval.cpp"), "haval.h"),
    (os.path.join(ALGO, "haval-helper.cpp"), "haval_helper.inc"),
    (os.path.join(ALGO, "md_helper.c"), "md_helper.inc"),
]
for name in [
    "Grindahl512", "Haval_5_256", "MD5", "RadioGatun32",
    "RIPEMD160", "RIPEMD256", "RIPEMD320", "SHA2_512", "SHA3_512",
    "Snefru_8_256", "Tiger2_5_192", "Whirlpool", "Ghost", "Ghost3411", "MD2",
    "MD4", "Panama", "HAS160", "SHA", "Grindahl256", "Haval", "RIPEMD",
    "RIPEMD128",
]:
    FILES.append((os.path.join(PARNIAN, "RandomHash_%s.h" % name), "rh_%s.h" % name.lower()))

KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof", "else", "do",
            "case", "defined", "RH_ASSERT", "memcpy", "memset"}

DEF_START = re.compile(r"^(?P<prefix>(?:[A-Za-z_][\w]*[\s\*]+)*)(?P<name>[A-Za-z_]\w*)\s*\(")


def is_definition(lines, i):
    """Return True if a function definition starts at line i (column 0)."""
    line = lines[i]
    if not line or line[0] in " \t#/{}*" or line.startswith("//"):
        return False
    if re.match(r"^(RH2_DEV\s+)?(struct|class|union|enum|typedef)\b", line):
        return False
    m = DEF_START.match(line)
    if not m or m.group("name") in KEYWORDS:
        return False
    # Walk forward: a '{' before any ';' means a definition
    depth = 0
    for j in range(i, min(i + 12, len(lines))):
        s = re.sub(r"//.*", "", lines[j])
        for ch in s:
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
            elif ch == ";" and depth == 0:
                return False
            elif ch == "{" and depth == 0:
                return True
            elif ch == "=" and depth == 0:
                return False
    return False


def port(text, fname):
    # includes of project files are handled by the master header
    text = re.sub(r'^\s*#include\s+"md_helper\.c"\s*$', '#include "md_helper.inc"', text, flags=re.M)
    text = re.sub(r'^\s*#include\s+"haval-helper\.cpp"\s*$', '#include "haval_helper.inc"', text, flags=re.M)
    text = re.sub(r'^\s*#include\s+"(MinersLib|corelib|precomp|sph|RandomHash)[^"]*"\s*$',
                  lambda m: "// (portable build) " + m.group(0).strip(), text, flags=re.M)
    text = re.sub(r'^\s*#include\s+<(stddef|string|stdint)\.h>\s*$', "", text, flags=re.M)

    # tables
    text = re.sub(r"\bstatic\s+PLATFORM_CONST\b", "RH2_CONST", text)
    text = re.sub(r"\bPLATFORM_CONST\b", "RH2_CONST", text)
    text = re.sub(r"^static\s+const\s+(?=[\w\s\(\)]*\[)", "RH2_CONST ", text, flags=re.M)
    text = re.sub(r"^static\s+const\s+(RH_ALIGN\(\d+\)\s+)?(?=\w+\s+(RH_ALIGN\(\d+\)\s+)?\w+\s*\[)",
                  "RH2_CONST ", text, flags=re.M)
    text = re.sub(r"^static const RH_ALIGN\(64\) U64", "RH2_CONST U64", text, flags=re.M)
    text = re.sub(r"^static const U64 RH_ALIGN\(64\)", "RH2_CONST U64", text, flags=re.M)
    text = re.sub(r"^RH_ALIGN\((\d+)\) static const unsigned char (\w+)\[\]", r"RH2_CONST RH_ALIGN(\1) unsigned char \2[]", text, flags=re.M)
    # mutable-looking but read-only global tables (gost s-boxes)
    text = re.sub(r"^U32 (gost_sbox_\d)\[256\] =", r"RH2_CONST U32 \1[256] =", text, flags=re.M)
    # sph style: static const sph_u32 NAME[...]
    text = re.sub(r"^static const (sph_u\d+|unsigned char|uint\d+_t|U\d+) (\w+)\[", r"RH2_CONST \1 \2[", text, flags=re.M)

    lines = text.split("\n")
    out = []
    for i, line in enumerate(lines):
        if is_definition(lines, i):
            m = DEF_START.match(line)
            if m.group("prefix").strip() == "" and out:
                # return type on the previous non-empty line (sph style)
                k = len(out) - 1
                while k >= 0 and out[k].strip() == "":
                    k -= 1
                if k >= 0 and not out[k].startswith("RH2_DEV") and re.match(r"^[A-Za-z_][\w\s\*]*$", out[k]):
                    out[k] = "RH2_DEV " + out[k]
                else:
                    line = "RH2_DEV " + line
            elif not line.startswith("RH2_DEV"):
                line = "RH2_DEV " + line
        out.append(line)
    text = "\n".join(out)

    # prototypes in sph headers and functions generated inside macros
    text = re.sub(r"^void \\$", lambda m: "RH2_DEV void \\", text, flags=re.M)
    text = re.sub(r"^RH2_DEV RH2_DEV ", "RH2_DEV ", text, flags=re.M)
    if fname.startswith("sph_"):
        text = re.sub(r"^void (\w+)\(", r"RH2_DEV void \1(", text, flags=re.M)

    # C linkage would leak the names out of the rh2 namespaces
    text = re.sub(r'#ifdef __cplusplus\s*\nextern "C"\s*\{\s*\n#endif', "", text)
    text = re.sub(r'#ifdef __cplusplus\s*\n\}\s*\n#endif', "", text)

    # SSE snippets -> scalar
    text = text.replace("__m128i xmm = _mm_set1_epi8((char)0); ", "")
    text = re.sub(r"RH_MM_STORE128\(\(\(__m128i \*\)(ctx->\w+)\)\+(\d)\s*,\s*xmm\);",
                  r"memset(((U8*)\1) + 16*\2, 0, 16);", text)
    return text


NAMESPACES = {
    "sph_sha0.h": "rh2_sha0", "sha0.h": "rh2_sha0",
    "sph_sha1.h": "rh2_sha1", "sha1.h": "rh2_sha1",
    "sph_panama.h": "rh2_panama", "panama.h": "rh2_panama",
    "sph_haval.h": "rh2_haval", "haval.h": "rh2_haval",
    "rh_haval_5_256.h": "rh2_haval5",
}


def wrap_namespace(text, ns):
    return ("namespace %s {\n" % ns) + text + ("\n} // namespace %s\nusing namespace %s;\n" % (ns, ns))


def main():
    os.makedirs(OUT, exist_ok=True)
    for src, dst in FILES:
        with open(src, "r", encoding="utf-8-sig", errors="replace") as f:
            text = f.read()
        ported = port(text, dst)
        if dst in NAMESPACES:
            ported = wrap_namespace(ported, NAMESPACES[dst])
        header = ("// Portable (host + CUDA device) copy generated by gpu-dev/port_algos.py\n"
                  "// from %s\n" % os.path.relpath(src, ROOT).replace("\\", "/"))
        with open(os.path.join(OUT, dst), "w", encoding="utf-8") as f:
            f.write(header + ported)
        print("ported", dst)


if __name__ == "__main__":
    sys.exit(main())
