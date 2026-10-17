# macOS pitfalls

## 'cstdint' file not found - stale sdk

Symptom: `'cstdint' file not found`
Probable cause: Removing the xcode SDK on macOS may create a situation where c++ headers cannot be found. The stale directory from an old Command Line Tools install therefore may hide the SDK's C++ headers. 
Solution: Move it away:

```sh
sudo mv /Library/Developer/CommandLineTools/usr/include/c++ ~/c++-stale-backup
```
