/**
 * @file window.hpp
 *
 * 表示領域を表すWindowクラスを提供する。
 */

#pragma once

#include <optional>
#include <vector>
#include "frame_buffer.hpp"
#include "graphics.hpp"

/** @brief Window クラスはグラフィックの表示領域を表す。
 *
 * タイトルやメニューがあるウィンドウだけでなく，マウスカーソルの表示領域なども対象とする。
 */
class Window {
public:
  /** @brief WindowWriter は Window と関連付けられた PixelWriter を提供する。
	 */
  class WindowWriter : public PixelWriter {
  public:
    WindowWriter(Window& window) : window_{window} {}

    /** @brief 指定された位置に指定された色を描く */
    virtual void Write(Vector2D<int> pos, const PixelColor& c) override {
      window_.Write(pos, c);
    }

    /** @brief Width は関連付けられた Window の横幅をピクセル単位で返す。 */
    virtual int Width() const override { return window_.Width(); }

    /** @brief Height は関連付けられた Window の高さをピクセル単位で返す。 */
    virtual int Height() const override { return window_.Height(); }

  private:
    Window& window_;
  };

  /** @brief 指定されたピクセル数の平面描画領域を作成する。 */
  Window(int width, int height, PixelFormat shadow_format);
  ~Window() = default;
  Window(const Window& rhs) = delete;
  Window& operator=(const Window& rhs) = delete;

  /** @brief 与えられた FrameBufferにこのウィンドウの表示領域を描画する。
   *
   * @param dst  描画先
   * @param pos  dst の左上を基準としたウィンドウの位置
   * @param area  dst の左上を基準とした描画対象範囲
   */
  void DrawTo(FrameBuffer& dst, Vector2D<int> pos, const Rectangle<int>& area);

  /** @brief 透過色を設定する。 */
  void SetTransparentColor(std::optional<PixelColor> c);

  /** @brief このインスタンスに紐付いた WindowWriter を取得する。 */
  WindowWriter* Writer();

  /** @brief 指定した位置のピクセルを返す。 */
  const PixelColor& At(Vector2D<int> pos) const;
  /** @brief 指定した位置にピクセルを書き込む。 */
  void Write(Vector2D<int> pos, PixelColor c);

  /** @brief 平面描画領域の横幅をピクセル単位で返す。 */
  int Width() const;
  /** @brief 平面描画領域の高さをピクセル単位で返す。 */
  int Height() const;
  /** @brief 平面描画領域のサイズをピクセル単位で返す。 */
  Vector2D<int> Size() const;

  /** @brief このウィンドウの平面描画領域内で，矩形領域を移動する。
   *
   * @param src_pos   移動元矩形の原点
   * @param src_size  移動元矩形の大きさ
   * @param dst_pos   移動先の原点
   */
  void Move(Vector2D<int> dst_pos, const Rectangle<int>& src);

private:
  int width_, height_;
  std::vector<std::vector<PixelColor>> data_{};
  WindowWriter writer_{*this};
  std::optional<PixelColor> transparent_color_{std::nullopt};

  FrameBuffer shadow_buffer_{};
};

void DrawWindow(PixelWriter& writer, const char* title);