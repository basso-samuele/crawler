# HTML Parser/Crawler

This project serves as a hands-on learning environment for understanding how browsers parse HTML documents, construct the Document Object Model (DOM), handle malformed markup, and more.

## Configure and build

```bash
# Configure with debug or release presets
cmake --preset debug   # or: cmake --preset release
cmake --build --preset debug   # or: cmake --preset release
```

## Run tests

```bash
# Run test suite with debug or release presets
ctest --preset debug   # or: ctest --preset release
```

## Current Capabilities

Given a basic HTML document such as:

```html
<html>
    <body>
        <h1>Hello, world!</h1>
        <p>This is a paragraph.</p>
    </body>
</html>
```

the parser can construct a corresponding simplified DOM tree representing the document structure:

```
Node type: root
Children:
        Node type: html
        Children:
                Node type: head
                Attributes:
                        profile -> main
                Children:
                        Node type: title
                        Attributes:
                                id -> title
                        Children:
                                Node type: text
                                Node value: Sample UTF-8 HTML Document
                Node type: body
                Children:
                        Node type: h1
                        Children:
                                Node type: text
                                Node value: Hello, world!
                        Node type: p
                        Children:
                                Node type: text
                                Node value: This is a paragraph.
```

I will dedicate a substantial amount of time to evaluating the project's structure, analyzing the current design and identifying potential flaws. I will then build upon this foundation to improve performance and make the architecture more flexible and easier to extend with new features.

### Performance (for future reference).
```
[21:11:21] [23905] [Crawler] [trace]: Scoped timer duration: 1521572ns (1521us, 1ms).
[21:11:17] [23899] [Crawler] [trace]: Scoped timer duration: 1732661ns (1732us, 1ms).
[21:11:13] [23893] [Crawler] [trace]: Scoped timer duration: 1676411ns (1676us, 1ms).
```
After removing multithreading.
```
[12:36:26] [17986] [Crawler] [trace]: Scoped timer duration: 1190353ns (1190us, 1ms).
[12:36:53] [18026] [Crawler] [trace]: Scoped timer duration: 1184723ns (1184us, 1ms).
[12:37:06] [18039] [Crawler] [trace]: Scoped timer duration: 1240813ns (1240us, 1ms).
```

## License

MIT License - see [LICENSE](./LICENSE) file for details.