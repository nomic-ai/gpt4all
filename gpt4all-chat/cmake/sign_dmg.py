#!/usr/bin/env python3
import os
import subprocess
import tempfile
import shutil
import click
from typing import Optional

# Requires click
# pip install click

# Example usage
# python sign_dmg.py --input-dmg /path/to/your/input.dmg --output-dmg /path/to/your/output.dmg --signing-identity "Developer ID Application: YOUR_NAME (TEAM_ID)"

# NOTE: This script assumes that you have the necessary Developer ID Application certificate in your
# Keychain Access and that the codesign and hdiutil command-line tools are available on your system.

@click.command()
@click.option('--input-dmg', required=True, help='Path to the input DMG file.')
@click.option('--output-dmg', required=True, help='Path to the output signed DMG file.')
@click.option('--sha1-hash', help='SHA-1 hash of the Developer ID Application certificate')
@click.option('--signing-identity', default=None, help='Common name of the Developer ID Application certificate')
def sign_dmg(input_dmg: str, output_dmg: str, signing_identity: Optional[str] = None, sha1_hash: Optional[str] = None) -> None:
    if not signing_identity and not sha1_hash:
        print("Error: Either --signing-identity or --sha1-hash must be provided.")
        exit(1)

    # Mount the input DMG
    mount_point = tempfile.mkdtemp()
    subprocess.run(['hdiutil', 'attach', input_dmg, '-mountpoint', mount_point])

    # Copy the contents of the DMG to a temporary folder
    temp_dir = tempfile.mkdtemp()
    shutil.copytree(mount_point, os.path.join(temp_dir, 'contents'))
    subprocess.run(['hdiutil', 'detach', mount_point])

    # Find the .app bundle in the temporary folder
    app_bundle = None
    for item in os.listdir(os.path.join(temp_dir, 'contents')):
        if item.endswith('.app'):
            app_bundle = os.path.join(temp_dir, 'contents', item)
            break

    if not app_bundle:
        print('No .app bundle found in the DMG.')
        exit(1)

    # Sign the .app bundle
    try:
        subprocess.run([
            'codesign',
            '--deep',
            '--force',
            '--verbose',
            '--options', 'runtime',
            '--timestamp',
            '--sign', sha1_hash or signing_identity,
            app_bundle
        ], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error during codesign: {e}")
        # Clean up temporary directories
        shutil.rmtree(temp_dir)
        shutil.rmtree(mount_point)
        exit(1)

    # Create a new DMG containing the signed .app bundle
    subprocess.run([
        'hdiutil', 'create',
        '-volname', os.path.splitext(os.path.basename(input_dmg))[0],
        '-srcfolder', os.path.join(temp_dir, 'contents'),
        '-ov',
        '-format', 'UDZO',
        output_dmg
    ])

    # Clean up temporary directories
    shutil.rmtree(temp_dir)
    shutil.rmtree(mount_point)

if __name__ == '__main__':
    sign_dmg()
