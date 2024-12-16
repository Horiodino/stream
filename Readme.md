# Stream
Stream is a simple OBS-like application for video capture.

## Installation

To install Stream, follow these steps:

1. Clone the repository:
    ```bash
    git clone https://github.com/Horiodino/stream.git
    ```
2. Navigate to the project directory:
    ```bash
    cd stream
    ```
3. Install the dependencies:
    ```bash
    sudo pacman -S base-devel xorgproto libx11 libxinerama libxext libxcomposite ffmpeg
    ```

## Usage

To start using Stream, run the following commands:
```bash
    make
    ./obs-capture
```

Sample: https://x.com/OdinHoli/status/1865014966870511838/video/2

Currently it does not have a graceful shutdown. You will need to review the code yourself and make the appropriate calls for recording and converting.
