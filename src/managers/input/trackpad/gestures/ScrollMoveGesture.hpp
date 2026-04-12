#pragma once

#include "ITrackpadGesture.hpp"

class CScrollMoveTrackpadGesture : public ITrackpadGesture {
  public:
    CScrollMoveTrackpadGesture()          = default;
    virtual ~CScrollMoveTrackpadGesture() = default;

    virtual void begin(const ITrackpadGesture::STrackpadGestureBegin& e);
    virtual void update(const ITrackpadGesture::STrackpadGestureUpdate& e);
    virtual void end(const ITrackpadGesture::STrackpadGestureEnd& e);

  private:
    bool m_wasScrollingLayout = false;
};
