/*
 * GeneralOptionsTab.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "GeneralOptionsTab.h"

#include "../../../lib/CGeneralTextHandler.h"
#include "../../../lib/filesystem/ResourceID.h"
#include "../../gui/CGuiHandler.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"
#include "../../widgets/Images.h"
#include "CGameInfo.h"
#include "CMusicHandler.h"
#include "CPlayerInterface.h"
#include "windows/GUIClasses.h"
#include "CServerHandler.h"
#include "renderSDL/SDL_Extensions.h"


static void setIntSetting(std::string group, std::string field, int value)
{
	Settings entry = settings.write[group][field];
	entry->Float() = value;
}

static void setBoolSetting(std::string group, std::string field, bool value)
{
	Settings fullscreen = settings.write[group][field];
	fullscreen->Bool() = value;
}

static std::string resolutionToString(int w, int h)
{
	return std::to_string(w) + 'x' + std::to_string(h);
}

GeneralOptionsTab::GeneralOptionsTab()
		: InterfaceObjectConfigurable(),
		  onFullscreenChanged(settings.listen["video"]["fullscreen"])
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	const JsonNode config(ResourceID("config/widgets/settings/generalOptionsTab.json"));
	addCallback("spellbookAnimationChanged", [](bool value)
	{
		return setBoolSetting("video", "spellbookAnimation", value);
	});
	addCallback("setMusic", [this](int value)
	{
		setIntSetting("general", "music", value);
		widget<CSlider>("musicSlider")->redraw();
	});
	addCallback("setVolume", [this](int value)
	{
		setIntSetting("general", "sound", value);
		widget<CSlider>("soundVolumeSlider")->redraw();
	});
	//settings that do not belong to base game:
	addCallback("fullscreenChanged", [this](bool value)
	{
		setFullscreenMode(value);
	});
	addCallback("setGameResolution", [this](int dummyValue)
	{
		selectGameResolution();
	});
	addCallback("framerateChanged", [](bool value)
	{
		setBoolSetting("general", "showfps", value);
	});
	//moved from "other" tab that is disabled for now to avoid excessible tabs with barely any content
	addCallback("availableCreaturesAsDwellingLabelChanged", [](bool value)
	{
		setBoolSetting("gameTweaks", "availableCreaturesAsDwellingLabel", value);
	});
	addCallback("compactTownCreatureInfoChanged", [](bool value)
	{
		return setBoolSetting("gameTweaks", "compactTownCreatureInfo", value);
	});
	build(config);

	std::shared_ptr<CLabel> resolutionLabel = widget<CLabel>("resolutionLabel");
	const auto & currentResolution = settings["video"]["screenRes"];
	resolutionLabel->setText(resolutionToString(currentResolution["width"].Integer(), currentResolution["height"].Integer()));


	std::shared_ptr<CToggleButton> spellbookAnimationCheckbox = widget<CToggleButton>("spellbookAnimationCheckbox");
	spellbookAnimationCheckbox->setSelected(settings["video"]["spellbookAnimation"].Bool());

	std::shared_ptr<CToggleButton> fullscreenCheckbox = widget<CToggleButton>("fullscreenCheckbox");
	fullscreenCheckbox->setSelected(settings["video"]["fullscreen"].Bool());
	onFullscreenChanged([&](const JsonNode &newState) //used when pressing F4 etc. to change fullscreen checkbox state
	{
		widget<CToggleButton>("fullscreenCheckbox")->setSelected(newState.Bool());
	});

	std::shared_ptr<CToggleButton> framerateCheckbox = widget<CToggleButton>("framerateCheckbox");
	framerateCheckbox->setSelected(settings["general"]["showfps"].Bool());

	std::shared_ptr<CSlider> musicSlider = widget<CSlider>("musicSlider");
	musicSlider->moveTo(CCS->musich->getVolume());

	std::shared_ptr<CSlider> volumeSlider = widget<CSlider>("soundVolumeSlider");
	volumeSlider->moveTo(CCS->soundh->getVolume());

	std::shared_ptr<CToggleButton> availableCreaturesAsDwellingLabelCheckbox = widget<CToggleButton>("availableCreaturesAsDwellingLabelCheckbox");
	availableCreaturesAsDwellingLabelCheckbox->setSelected(settings["gameTweaks"]["availableCreaturesAsDwellingLabel"].Bool());

	std::shared_ptr<CToggleButton> compactTownCreatureInfo = widget<CToggleButton>("compactTownCreatureInfoCheckbox");
	compactTownCreatureInfo->setSelected(settings["gameTweaks"]["compactTownCreatureInfo"].Bool());
}


bool GeneralOptionsTab::isResolutionSupported(const Point & resolution)
{
	return isResolutionSupported( resolution, settings["video"]["fullscreen"].Bool());
}

bool GeneralOptionsTab::isResolutionSupported(const Point & resolution, bool fullscreen)
{
	if (!fullscreen)
		return true;

	auto supportedList = CSDL_Ext::getSupportedResolutions();

	return CSDL_Ext::isResolutionSupported(supportedList, resolution);
}

void GeneralOptionsTab::selectGameResolution()
{
	fillSelectableResolutions();

	std::vector<std::string> items;
	size_t currentResolutionIndex = 0;
	size_t i = 0;
	for(const auto & it : selectableResolutions)
	{
		auto resolutionStr = resolutionToString(it.x, it.y);
		if(widget<CLabel>("resolutionLabel")->getText() == resolutionStr)
			currentResolutionIndex = i;

		items.push_back(std::move(resolutionStr));
		++i;
	}
	GH.pushIntT<CObjectListWindow>(items, nullptr,
								   CGI->generaltexth->translate("vcmi.systemOptions.resolutionMenu.hover"),
								   CGI->generaltexth->translate("vcmi.systemOptions.resolutionMenu.help"),
								   [this](int index)
								   {
									   setGameResolution(index);
								   },
								   currentResolutionIndex);
}

void GeneralOptionsTab::setGameResolution(int index)
{
	assert(index >= 0 && index < selectableResolutions.size());

	if ( index < 0 || index >= selectableResolutions.size() )
		return;

	Point resolution = selectableResolutions[index];

	Settings gameRes = settings.write["video"]["screenRes"];
	gameRes["width"].Float() = resolution.x;
	gameRes["height"].Float() = resolution.y;

	std::string resText;
	resText += std::to_string(resolution.x);
	resText += "x";
	resText += std::to_string(resolution.y);
	widget<CLabel>("resolutionLabel")->setText(resText);
}

void GeneralOptionsTab::setFullscreenMode(bool on)
{
	fillSelectableResolutions();

	const auto & screenRes = settings["video"]["screenRes"];
	const Point desiredResolution(screenRes["width"].Integer(), screenRes["height"].Integer());
	const Point currentResolution = GH.screenDimensions();

	if (!isResolutionSupported(currentResolution, on))
	{
		widget<CToggleButton>("fullscreenCheckbox")->setSelected(!on);
		LOCPLINT->showInfoDialog(CGI->generaltexth->translate("vcmi.systemOptions.fullscreenFailed"));
		return;
	}

	setBoolSetting("video", "fullscreen", on);

	if (!isResolutionSupported(desiredResolution, on))
	{
		// user changed his desired resolution and switched to fullscreen
		// however resolution he selected before is not available in fullscreen
		// so reset it back to currect resolution which is confirmed to be supported earlier
		Settings gameRes = settings.write["video"]["screenRes"];
		gameRes["width"].Float() = currentResolution.x;
		gameRes["height"].Float() = currentResolution.y;

		widget<CLabel>("resolutionLabel")->setText(resolutionToString(currentResolution.x, currentResolution.y));
	}
}

void GeneralOptionsTab::fillSelectableResolutions()
{
	selectableResolutions.clear();

	for(const auto & it : conf.guiOptions)
	{
		const Point dimensions(it.first.first, it.first.second);

		if(isResolutionSupported(dimensions))
			selectableResolutions.push_back(dimensions);
	}

	boost::range::sort(selectableResolutions, [](const auto & left, const auto & right)
	{
		return left.x * left.y < right.x * right.y;
	});
}