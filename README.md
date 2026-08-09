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

## License

MIT License - see [LICENSE](./LICENSE) file for details.