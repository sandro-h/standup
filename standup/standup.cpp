#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#include <iostream>
#include <chrono>
#include <format>
#include <thread>
#include <fstream>
#include <string>
#include "wintoastlib.h"

#include "image.h"

using namespace std::chrono;
using namespace WinToastLib;

class MyWinToastHandler : public IWinToastHandler {
public:
	void toastActivated() const override {};
	void toastActivated(int actionIndex) const override {};
	void toastDismissed(WinToastDismissalReason state) const override {};
	void toastFailed() const override {};
};

auto getNow() {
	return current_zone()->to_local(system_clock::now());
}

auto getFirstTime(int interval) {
	auto now = getNow();

	auto nextTime = floor<hours>(now) + minutes(interval);
	while (nextTime <= now)
	{
		nextTime += minutes(interval);
	}

	return nextTime;
}

void popup(const std::wstring& title, const std::wstring& message, const std::wstring& imagePath, WinToastTemplate::Duration duration)
{
	WinToastTemplate templ(WinToastTemplate::ImageAndText02);
	templ.setTextField(title, WinToastTemplate::FirstLine);
	templ.setTextField(message, WinToastTemplate::SecondLine);
	templ.setImagePath(imagePath);
	templ.setDuration(duration);

	auto handler = new MyWinToastHandler();
	WinToast::WinToastError error;
	if (WinToast::instance()->showToast(templ, handler, &error) < 0)
	{
		std::wcout << L"Error showing toast: " << error << std::endl;
	}
}

int main(int argc, char* argv[])
{
	// Extract image
	wchar_t buf[MAX_PATH];
	GetTempPath(MAX_PATH, buf);
	auto imagePath = std::wstring(buf) + L"standup.png";

	std::ofstream wf(imagePath, std::ios::out | std::ios::binary);
	wf.write((char*)&standup_png[0], standup_png_len);
	wf.close();

	// Prepare WinToast
	WinToast::WinToastError error;
	WinToast::instance()->setAppName(L"standup");
	const auto aumi = WinToast::configureAUMI(L"standup", L"standup", L"standup", L"20230812");
	WinToast::instance()->setAppUserModelId(aumi);

	if (!WinToast::instance()->initialize(&error))
	{
		wchar_t buf[250];
		swprintf_s(buf, L"Failed to initialize WinToast :%d", error);
		std::wcout << buf << std::endl;
	}

	// Main app logic
	int interval = argc > 1 ? std::stoi(argv[1]) : 60;

	auto nextTime = getFirstTime(interval);

	popup(L"standup started!",
		std::format(L"Reminding every {} minutes. Next: {:%H:%M}", interval, nextTime),
		imagePath,
		WinToastTemplate::Duration::Short
	);

	while (true)
	{
		if (getNow() >= nextTime)
		{
			auto thisTime = nextTime;
			nextTime += minutes(interval);
			popup(L"Stand up!",
				std::format(L"It's {:%H:%M}! Stand up, walk around and stretch for a while!", thisTime),
				imagePath,
				WinToastTemplate::Duration::Long
			);
		}
		std::this_thread::sleep_for(seconds(5));
	}
}
