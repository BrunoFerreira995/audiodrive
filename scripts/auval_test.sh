#!/bin/bash
# Run auval on built AudioUnit component if available
COMPONENT="${1:-build-au/plugins/Audio32Plugin_artefacts/Release/AU/Audio32 Effects.component}"
if [ ! -d "$COMPONENT" ]; then
    echo "Component not found: $COMPONENT"
    exit 1
fi
if command -v auval >/dev/null 2>&1; then
    auval -v aufx Au32 A032 "$COMPONENT"
else
    echo "auval not found"
    exit 1
fi
