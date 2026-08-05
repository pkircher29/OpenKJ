#include "cdg/cdgimageframe.h"

#include <QByteArray>
#include <iostream>

namespace {

bool require(bool condition, const char *message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}

cdg::CDG_SubCode scrollCommand(cdg::CdgCommand instruction, char horizontal, char vertical)
{
    cdg::CDG_SubCode command{};
    command.command = static_cast<cdg::CdgCommand>(0x09);
    command.instruction = instruction;
    command.data[0] = 1;
    command.data[1] = horizontal;
    command.data[2] = vertical;
    return command;
}

} // namespace

int main()
{
    std::array<char, 16> malformed{};
    malformed[1] = 0x17; // right-scroll command with invalid offset 7
    malformed[2] = 0x1f; // down-scroll command with invalid offset 15
    const cdg::CdgScrollCmdData parsed(malformed);

    bool ok = true;
    ok &= require(parsed.hSOffset == 5, "horizontal CDG offset was not clamped to 5");
    ok &= require(parsed.vSOffset == 11, "vertical CDG offset was not clamped to 11");

    CdgImageFrame frame;
    ok &= require(frame.applySubCode(scrollCommand(cdg::CmdScrollCopy, 0x17, 0x1f)),
                  "malformed scroll-copy packet was not handled");
    ok &= require(frame.applySubCode(scrollCommand(cdg::CmdScrollPreset, 0x20, 0x20)),
                  "left/up scroll-preset packet was not handled");
    ok &= require(frame.applySubCode(scrollCommand(cdg::CmdScrollCopy, 0x10, 0x10)),
                  "right/down scroll-copy packet was not handled");

    QByteArray cropped(cdg::CDG_IMAGE_SIZE, 0);
    frame.copyCroppedImagedata(reinterpret_cast<uchar *>(cropped.data()));
    ok &= require(!frame.getImage().isNull(), "CDG image became invalid after scrolling");

    CdgImageFrame leftPresetFrame;
    auto leftPreset = scrollCommand(cdg::CmdScrollPreset, 0x20, 0x00);
    leftPreset.data[0] = 5;
    ok &= require(leftPresetFrame.applySubCode(leftPreset), "left scroll-preset packet was not handled");
    const QImage leftImage = leftPresetFrame.getImage();
    ok &= require(leftImage.pixelIndex(294, 0) == 5 && leftImage.pixelIndex(299, 0) == 5,
                  "left scroll preset did not fill the vacated rightmost pixels");
    ok &= require(leftImage.pixelIndex(6, 0) == 0,
                  "left scroll preset corrupted pixels near the left edge");

    CdgImageFrame rightPresetFrame;
    auto rightPreset = scrollCommand(cdg::CmdScrollPreset, 0x10, 0x00);
    rightPreset.data[0] = 6;
    ok &= require(rightPresetFrame.applySubCode(rightPreset), "right scroll-preset packet was not handled");
    const QImage rightImage = rightPresetFrame.getImage();
    ok &= require(rightImage.pixelIndex(0, 0) == 6 && rightImage.pixelIndex(5, 0) == 6,
                  "right scroll preset did not fill the vacated leftmost pixels");
    ok &= require(rightImage.pixelIndex(6, 0) == 0,
                  "right scroll preset overwrote non-vacated pixels");

    if (ok)
        std::cout << "PASS: CDG malformed-offset and scroll regression checks\n";
    return ok ? 0 : 1;
}
