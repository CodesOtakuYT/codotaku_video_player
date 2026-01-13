#include <SDL3/SDL.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

int main(void) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer("Codotaku video player", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer);
    SDL_MaximizeWindow(window);

    AVFormatContext *format_context = NULL;
    avformat_open_input(&format_context,
                        "/home/codotaku/Downloads/YTDown.com_YouTube_Big-Buck-Bunny-60fps-4K-Official-Blender_Media_aqz-KE-bpKQ_001_1080p.mp4",
                        NULL, NULL);
    const AVCodec *codec = NULL;
    const int video_stream_index = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    AVCodecContext *decoder = avcodec_alloc_context3(codec);
    decoder->thread_count = 0;
    avcodec_parameters_to_context(decoder, format_context->streams[video_stream_index]->codecpar);
    avcodec_open2(decoder, codec, NULL);

    AVPacket packet;
    AVFrame *frame = av_frame_alloc();

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING,
                                             decoder->width,
                                             decoder->height);

    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) if (event.type == SDL_EVENT_QUIT) return EXIT_SUCCESS;

        if (av_read_frame(format_context, &packet) >= 0) {
            if (packet.stream_index == video_stream_index) {
                avcodec_send_packet(decoder, &packet);
                if (avcodec_receive_frame(decoder, frame) == 0) {
                    SDL_UpdateYUVTexture(texture, NULL, frame->data[0], frame->linesize[0], frame->data[1],
                                         frame->linesize[1], frame->data[2], frame->linesize[2]);
                }
            }
            av_packet_unref(&packet);
        }

        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
}
