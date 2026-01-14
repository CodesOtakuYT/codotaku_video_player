// ignore error handling, cleanup and flushing since we're quitting
#include <SDL3/SDL.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

int main(int argc, char **argv) {
    AVFormatContext *format_context = NULL;
    avformat_open_input(&format_context,
                        argv[1],
                        NULL, NULL);
    avformat_find_stream_info(format_context, NULL);
    const AVCodec *codec = NULL;
    const int video_stream_index = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    const AVStream *video_stream = format_context->streams[video_stream_index];
    for (int stream_index = 0; stream_index < format_context->nb_streams; stream_index++)
        if (stream_index != video_stream_index)
            format_context->streams[stream_index]->discard = AVDISCARD_ALL;

    AVCodecContext *decoder = avcodec_alloc_context3(codec);
    decoder->thread_count = 0;
    avcodec_parameters_to_context(decoder, video_stream->codecpar);
    avcodec_open2(decoder, codec, NULL);
    SDL_assert(decoder->pix_fmt == AV_PIX_FMT_YUV420P);

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer("Codotaku video player", decoder->width, decoder->height, SDL_WINDOW_RESIZABLE, &window,
                                &renderer);
    SDL_SetRenderVSync(renderer, 1);

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING,
                                             decoder->width,
                                             decoder->height);

    Uint64 start_ns = 0;
    const double timebase = av_q2d(video_stream->time_base);

    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) if (event.type == SDL_EVENT_QUIT) return EXIT_SUCCESS;

        if (av_read_frame(format_context, packet) >= 0) {
            if (packet->stream_index == video_stream_index) {
                avcodec_send_packet(decoder, packet);
                while (avcodec_receive_frame(decoder, frame) == 0) {
                    bool skip_frame = false;
                    if (start_ns == 0) start_ns = SDL_GetTicksNS();
                    while (true) {
                        const Uint64 elapsed_time_ns = SDL_GetTicksNS() - start_ns;
                        const double elapsed_time_s = (double) elapsed_time_ns / SDL_NS_PER_SECOND;
                        const double frame_time_s = (double) frame->best_effort_timestamp * timebase;
                        const double delay_s = frame_time_s - elapsed_time_s;
                        if (delay_s < -0.05)
                            skip_frame = true;

                        if (delay_s <= 0.0)
                            break;

                        if (delay_s > 0.01)
                            SDL_Delay(1);
                    }

                    if (skip_frame)
                        continue;

                    SDL_UpdateYUVTexture(texture, NULL, frame->data[0], frame->linesize[0], frame->data[1],
                                         frame->linesize[1], frame->data[2], frame->linesize[2]);

                    int w, h;
                    SDL_GetCurrentRenderOutputSize(renderer, &w, &h);
                    const float frame_width = (float) decoder->width;
                    const float frame_height = (float) decoder->height;
                    const float scale_w = (float) w / frame_width;
                    const float scale_h = (float) h / frame_height;
                    const float scale = SDL_min(scale_w, scale_h);
                    SDL_FRect dstrect;
                    dstrect.w = frame_width * scale;
                    dstrect.h = frame_height * scale;
                    dstrect.x = ((float) w - dstrect.w) / 2;
                    dstrect.y = ((float) h - dstrect.h) / 2;

                    SDL_RenderTexture(renderer, texture, NULL, &dstrect);
                    SDL_RenderPresent(renderer);
                }
            }
            av_packet_unref(packet);
        } else
            break;
    }
}
