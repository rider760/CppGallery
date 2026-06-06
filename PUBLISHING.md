# Publishing To GitHub

This file describes the shortest path from the staged open-source snapshot to a public GitHub repository.

## 1. Set your real git identity

The staged repository may use a placeholder local identity so the first commit can exist locally.
Before pushing publicly, set your real identity inside the staged repository:

```powershell
git config user.name "Your Name"
git config user.email "you@example.com"
git commit --amend --reset-author --no-edit
```

## 2. Create an empty GitHub repository

Create a new empty repository on GitHub.

Important:

- do not add a README
- do not add a LICENSE
- do not add a `.gitignore`

The local staged repository already contains those files.

## 3. Add the remote and push

Inside `publish/open-source/CppGallery/`:

```powershell
git remote add origin https://github.com/<your-name>/CppGallery.git
git push -u origin main
```

## 4. Add screenshots

Recommended screenshot files:

- `docs/screenshots/gallery-overview.png`
- `docs/screenshots/zoom-view.png`
- `docs/screenshots/start-pinned-launcher.png`

Commit and push them after adding the images.

## 5. Publish binaries separately

Keep the source repository clean.
If you want to ship Windows binaries:

1. build a Release package
2. keep FFmpeg binaries external unless you are handling their distribution obligations yourself
3. upload the packaged executable to GitHub Releases instead of committing binaries into the repository
