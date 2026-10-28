#!/bin/bash
# Code signing and notarization template for macOS
# Requires: APPLE_ID, TEAM_ID, APP_PASSWORD, SIGNING_IDENTITY
COMPONENT="$1"
IDENTITY="${SIGNING_IDENTITY:-Developer ID Application}"
if [ -z "$COMPONENT" ] || [ ! -d "$COMPONENT" ]; then
    echo "Usage: $0 <path-to.component>"
    exit 1
fi
codesign --force --deep --strict --timestamp --options runtime -s "$IDENTITY" "$COMPONENT"
codesign -dv --verbose=4 "$COMPONENT"
echo "Create zip for notarization..."
ditto -c -k --keepParent "$COMPONENT" "${COMPONENT}.zip"
if [ -n "$APPLE_ID" ] && [ -n "$TEAM_ID" ] && [ -n "$APP_PASSWORD" ]; then
    xcrun notarytool submit "${COMPONENT}.zip" --apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$APP_PASSWORD" --wait
    xcrun stapler staple "$COMPONENT"
else
    echo "Set APPLE_ID, TEAM_ID, APP_PASSWORD for notarization"
fi
rm "${COMPONENT}.zip"
