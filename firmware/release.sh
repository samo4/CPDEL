#!/bin/bash

get_next_version() {
  latest_tag=$(git tag -l "v[0-9]*.[0-9]*" | sort -V | tail -n 1)
  
  if [ -z "$latest_tag" ]; then
    echo "0.1"
  else
    major=$(echo $latest_tag | cut -d'.' -f1 | cut -d'v' -f2)
    minor=$(echo $latest_tag | cut -d'.' -f2)
    
    next_minor=$((minor + 1))
    
    echo "${major}.${next_minor}"
  fi
}

# Get version from argument or auto-increment
if [ -z "$1" ]; then
  VERSION=$(get_next_version)
else
  VERSION=$1
  if [[ ! $VERSION =~ ^[0-9]+\.[0-9]+$ ]]; then
    echo "Error: Version tag must match the format x.y (e.g., 1.0)"
    exit 1
  fi
fi

# Optional release notes (2nd argument), embedded in the annotated tag.
if [ -z "$2" ]; then
  NOTES="Release v$VERSION"
else
  NOTES=$2
fi

# Check if there are changes to commit
if git diff-index --quiet HEAD --; then
  echo "No changes to commit."
  exit 0
fi

# Commit changes, create tag, and push
echo "Creating release v$VERSION..."
git add .
git commit -m "v$VERSION"
git tag -a "v$VERSION" -m "$NOTES"
git push --tags
git push

echo "Release v$VERSION created and pushed successfully."
echo "GitHub Actions will build the firmware and the report PDF automatically."
