// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/themes/theme_service.h"

#include "base/prefs/pref_service.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"

namespace {

// The toolbar color specified in the theme.
const SkColor kThemeToolbarColor = 0xFFCFDDC0;

bool UsingCustomTheme(const ThemeService& theme_service) {
  return !theme_service.UsingNativeTheme() &&
         !theme_service.UsingDefaultTheme();
}

typedef ExtensionBrowserTest ThemeServiceBrowserTest;

// Test that the theme is recreated from the extension when the data pack is
// unavailable or invalid (such as when the theme pack version is incremented).
// The PRE_ part of the test installs the theme and changes where Chrome looks
// for the theme data pack to make sure that Chrome does not find it.
IN_PROC_BROWSER_TEST_F(ThemeServiceBrowserTest, PRE_ThemeDataPackInvalid) {
  Profile* profile = browser()->profile();
  ThemeService* theme_service = ThemeServiceFactory::GetForProfile(profile);

  // Test initial state.
  EXPECT_FALSE(UsingCustomTheme(*theme_service));
  EXPECT_NE(kThemeToolbarColor, theme_service->GetColor(
      ThemeProperties::COLOR_TOOLBAR));
  EXPECT_EQ(base::FilePath(),
            profile->GetPrefs()->GetFilePath(prefs::kCurrentThemePackFilename));

  InstallExtension(test_data_dir_.AppendASCII("theme"), 1);

  // Check that the theme was installed.
  EXPECT_TRUE(UsingCustomTheme(*theme_service));
  EXPECT_EQ(kThemeToolbarColor, theme_service->GetColor(
      ThemeProperties::COLOR_TOOLBAR));
  EXPECT_NE(base::FilePath(),
            profile->GetPrefs()->GetFilePath(prefs::kCurrentThemePackFilename));

  // Change the theme data pack path to an invalid location such that second
  // part of the test is forced to recreate the theme pack when the theme
  // service is initialized.
  profile->GetPrefs()->SetFilePath(
      prefs::kCurrentThemePackFilename,
      base::FilePath());
}

IN_PROC_BROWSER_TEST_F(ThemeServiceBrowserTest, ThemeDataPackInvalid) {
  ThemeService* theme_service = ThemeServiceFactory::GetForProfile(
      browser()->profile());
  EXPECT_TRUE(UsingCustomTheme(*theme_service));
  EXPECT_EQ(kThemeToolbarColor, theme_service->GetColor(
      ThemeProperties::COLOR_TOOLBAR));
}

}  // namespace
