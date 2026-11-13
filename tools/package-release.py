# Rad Pro
# Packages signed firmware into a release zip, reusing the lib/, backup/,
# README.md and per-device install scripts of an upstream release zip.

import argparse
import glob
import os
import sys
import zipfile


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--upstream-zip", required=True)
    p.add_argument("--signed-dir", required=True)
    p.add_argument("--version", required=True)
    p.add_argument("--out", required=True)
    args = p.parse_args()

    new = "radpro-" + args.version + "/"

    devices = {}
    for d in sorted(glob.glob(os.path.join(args.signed_dir, "*", "firmware"))):
        device = os.path.basename(os.path.dirname(d))
        bins = sorted(glob.glob(os.path.join(d, "*.bin")))
        if bins:
            devices[device] = bins
    if not devices:
        sys.exit("no signed firmware found in " + args.signed_dir)

    with zipfile.ZipFile(args.upstream_zip) as zin:
        tops = {n.split("/")[0] for n in zin.namelist()}
        if len(tops) != 1:
            sys.exit("unexpected upstream layout: " + str(sorted(tops)))
        old = tops.pop() + "/"

        keep_prefixes = [old + "lib/", old + "backup/"]
        keep_exact = {old, old + "README.md"}
        for device in devices:
            if old + device + "/" not in zin.namelist():
                sys.exit("device folder missing in upstream zip: " + device)
            keep_exact.add(old + device + "/")
            keep_prefixes.append(old + device + "/install.")
        keep_prefixes = tuple(keep_prefixes)

        os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
        with zipfile.ZipFile(args.out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zout:
            for info in zin.infolist():
                name = info.filename
                if name in keep_exact or name.startswith(keep_prefixes):
                    out = zipfile.ZipInfo(new + name[len(old):], date_time=info.date_time)
                    out.external_attr = info.external_attr
                    out.create_system = info.create_system
                    is_dir = name.endswith("/")
                    out.compress_type = zipfile.ZIP_STORED if is_dir else zipfile.ZIP_DEFLATED
                    zout.writestr(out, b"" if is_dir else zin.read(name))
            for device, bins in devices.items():
                d = zipfile.ZipInfo(new + device + "/firmware/")
                d.external_attr = (0o40755 << 16) | 0x10
                d.create_system = 3
                zout.writestr(d, b"")
                for b in bins:
                    f = zipfile.ZipInfo(new + device + "/firmware/" + os.path.basename(b))
                    f.external_attr = 0o100644 << 16
                    f.create_system = 3
                    f.compress_type = zipfile.ZIP_DEFLATED
                    with open(b, "rb") as fh:
                        zout.writestr(f, fh.read())

    with zipfile.ZipFile(args.out) as z:
        names = z.namelist()
        print(len(names), "entries")
        for n in names:
            if not n.startswith(new + "lib/"):
                print(" ", n)
        print("exec sh:", [(i.filename, oct(i.external_attr >> 16)) for i in z.infolist() if i.filename.endswith(".sh")])


main()
