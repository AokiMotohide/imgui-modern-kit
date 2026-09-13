#!/bin/sh
set -eu
app_path="$1"
: "${APPLE_SIGNING_IDENTITY:?APPLE_SIGNING_IDENTITY is required}"
: "${APPLE_NOTARY_PROFILE:?APPLE_NOTARY_PROFILE is required}"
codesign --force --deep --options runtime --timestamp --sign "$APPLE_SIGNING_IDENTITY" "$app_path"
archive_path="${app_path%.app}-notarization.zip"
ditto -c -k --keepParent "$app_path" "$archive_path"
xcrun notarytool submit "$archive_path" --keychain-profile "$APPLE_NOTARY_PROFILE" --wait
xcrun stapler staple "$app_path"
codesign --verify --deep --strict --verbose=2 "$app_path"
