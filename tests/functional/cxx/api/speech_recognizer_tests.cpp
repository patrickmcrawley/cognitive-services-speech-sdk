//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.md file in the project root for full license information.
//

#include <iostream>
#include <atomic>
#include <map>
#include <string>

#include "test_utils.h"
#include "file_utils.h"

#include "exception.h"
#define __SPX_THROW_HR_IMPL(hr) Microsoft::CognitiveServices::Speech::Impl::ThrowWithCallstack(hr)

#include "speechapi_cxx.h"
#include "mock_controller.h"

using namespace Microsoft::CognitiveServices::Speech::Impl; // for mocks

using namespace Microsoft::CognitiveServices::Speech;
using namespace Microsoft::CognitiveServices::Speech::Audio;
using namespace std;

static string input_file("tests/input/whatstheweatherlike.wav");

static std::shared_ptr<SpeechConfig> CurrentSpeechConfig()
{
    return !Config::Endpoint.empty()
        ? SpeechConfig::FromEndpoint(Config::Endpoint, Keys::Speech)
        : SpeechConfig::FromSubscription(Keys::Speech, Config::Region);
}

template< typename RecogType>
static std::shared_ptr<RecogType> CreateRecognizers(const string& filename)
{
    auto audioInput = AudioConfig::FromWavFileInput(filename);
    return RecogType::FromConfig(CurrentSpeechConfig(), audioInput);
}

void UseMocks(bool value)
{
    SpxSetMockParameterBool("CARBON-INTERNAL-MOCK-UspRecoEngine", value);
    SpxSetMockParameterBool("CARBON-INTERNAL-MOCK-Microphone", value);
    SpxSetMockParameterBool("CARBON-INTERNAL-MOCK-SdkKwsEngine", value);
}

void UseMockUsp(bool value)
{
    SpxSetMockParameterBool("CARBON-INTERNAL-MOCK-UspRecoEngine", value);
}

bool IsUsingMocks(bool uspMockRequired = true)
{
    return SpxGetMockParameterBool("CARBON-INTERNAL-MOCK-Microphone") &&
           SpxGetMockParameterBool("CARBON-INTERNAL-MOCK-SdkKwsEngine") &&
           (SpxGetMockParameterBool("CARBON-INTERNAL-MOCK-UspRecoEngine") || !uspMockRequired);
}

void SetMockRealTimeSpeed(int value)
{
    SpxSetMockParameterNumber("CARBON-INTERNAL-MOCK-RealTimeAudioPercentage", value);
}


enum class Callbacks { final_result, intermediate_result, no_match, session_started, session_stopped, speech_start_detected, speech_end_detected };

std::map<Callbacks, atomic_int> createCallbacksMap() {
    std::map<Callbacks, atomic_int> newMap;
    newMap[Callbacks::final_result] = 0;
    newMap[Callbacks::intermediate_result] = 0;
    newMap[Callbacks::no_match] = 0;
    newMap[Callbacks::session_started] = 0;
    newMap[Callbacks::speech_start_detected] = 0;
    newMap[Callbacks::session_stopped] = 0;
    return newMap;
}


TEST_CASE("Speech Recognizer basics", "[api][cxx]")
{
    SPX_TRACE_SCOPE(__FUNCTION__, __FUNCTION__);

    SECTION("Check that recognizer does not crash while async op is in progress")
    {
        UseMocks(true);
        REQUIRE(IsUsingMocks());

        const int numLoops = 10;

        {
            vector<std::future<std::shared_ptr<SpeechRecognitionResult>>> futures;
            for (int i = 0; i < numLoops; i++)
            {
                auto recognizer = SpeechRecognizer::FromConfig(CurrentSpeechConfig());
                REQUIRE(recognizer != nullptr);
                futures.push_back(recognizer->RecognizeAsync());
            }
        }

        auto model = KeywordRecognitionModel::FromFile("tests/input/heycortana_en-US.table");
        REQUIRE(model != nullptr);

        {
            vector<std::future<void>> futures;
            int numAsyncMethods = 4;
            for (int i = 0; i < numLoops*numAsyncMethods; i++)
            {
                auto recognizer = SpeechRecognizer::FromConfig(CurrentSpeechConfig());
                REQUIRE(recognizer != nullptr);
                if (i % numAsyncMethods == 0)
                {
                    futures.push_back(recognizer->StartContinuousRecognitionAsync());
                }
                else if (i % numAsyncMethods == 1)
                {
                    futures.push_back(recognizer->StopContinuousRecognitionAsync());
                }
                else if (i % numAsyncMethods == 2)
                {
                    futures.push_back(recognizer->StartKeywordRecognitionAsync(model));
                }
                else
                {
                    futures.push_back(recognizer->StopKeywordRecognitionAsync());
                }
            }
        }
    }

    GIVEN("Mocks for USP, Microphone, WavFilePump and Reader, and then USP ...")
    {
        SPX_TRACE_VERBOSE("%s: line=%d", __FUNCTION__, __LINE__);

        UseMocks(true);

        REQUIRE(exists(PAL::ToWString(input_file)));

        mutex mtx;
        condition_variable cv;

        WHEN("We checking to make sure callback counts are correct (checking multiple times, and multiple speeds times) ...")
        {
            std::map<Callbacks, atomic_int> callbackCounts = createCallbacksMap();

            // We're going to loop thru 11 times... The first 10, we'll use mocks. The last time we'll use the USP
            // NOTE: Please keep this at 11... It tests various "race"/"speed" configurations of the core system... 
            // NOTE: When running agains the localhost, loop 20 times... Half the time, we'll use mocks, and half - the USP.
            const int numLoops = (Config::Endpoint.empty()) ? 11 : 20;

            for (int i = 0; i < numLoops; i++)
            {
                auto useMockUsp = true;

                if (!Config::Endpoint.empty() && i % 2 == 0)
                {
                    useMockUsp = false;
                }
                else
                {
                    useMockUsp = i + 1 < numLoops;
                }

                auto realTimeRate = (i + 1) * 100 / numLoops;
                SetMockRealTimeSpeed(realTimeRate);
                UseMockUsp(useMockUsp);

                SPX_TRACE_VERBOSE("%s: START of loop #%d; mockUsp=%d; realtime=%d", __FUNCTION__, i, useMockUsp, realTimeRate);

                auto recognizer = CreateRecognizers<SpeechRecognizer>(input_file);
                REQUIRE(recognizer != nullptr);
                REQUIRE(IsUsingMocks(useMockUsp));

                bool sessionEnded = false;

                recognizer->FinalResult.Connect([&](const SpeechRecognitionEventArgs&) {
                    callbackCounts[Callbacks::final_result]++;
                    SPX_TRACE_VERBOSE("callbackCounts[Callbacks::final_result]=%d", callbackCounts[Callbacks::final_result].load());
                });
                recognizer->SessionStarted.Connect([&](const SessionEventArgs&) {
                    callbackCounts[Callbacks::session_started]++;
                    SPX_TRACE_VERBOSE("callbackCounts[Callbacks::session_started]=%d", callbackCounts[Callbacks::session_started].load());
                });
                recognizer->SpeechStartDetected.Connect([&](const RecognitionEventArgs&) {
                    callbackCounts[Callbacks::speech_start_detected]++;
                    SPX_TRACE_VERBOSE("callbackCounts[Callbacks::speech_start_detected]=%d", callbackCounts[Callbacks::speech_start_detected].load());
                });
                recognizer->SpeechEndDetected.Connect([&](const RecognitionEventArgs&) {
                    callbackCounts[Callbacks::speech_end_detected]++;
                    SPX_TRACE_VERBOSE("callbackCounts[Callbacks::speech_end_detected]=%d", callbackCounts[Callbacks::speech_end_detected].load());
                });
                recognizer->SessionStopped.Connect([&](const SessionEventArgs&) {
                    callbackCounts[Callbacks::session_stopped]++;
                    SPX_TRACE_VERBOSE("callbackCounts[Callbacks::session_stopped]=%d", callbackCounts[Callbacks::session_stopped].load());
                    unique_lock<mutex> lock(mtx);
                    sessionEnded = true;
                    cv.notify_one();
                });

                auto result = recognizer->RecognizeAsync().get();
                CHECK(result != nullptr);

                SPX_TRACE_VERBOSE("%s: Wait for session end (loop #%d)", __FUNCTION__, i);
                unique_lock<mutex> lock(mtx);
                cv.wait_for(lock, std::chrono::seconds(30), [&] { return sessionEnded; });
                lock.unlock();

                SPX_TRACE_VERBOSE("%s: Make sure callbacks are invoked correctly; END of loop #%d", __FUNCTION__, i);
                CHECK(callbackCounts[Callbacks::session_started] == i+1);
                CHECK(callbackCounts[Callbacks::session_stopped] == i + 1);
                CHECK(callbackCounts[Callbacks::final_result] == i + 1);
                CHECK(callbackCounts[Callbacks::speech_start_detected] == i + 1);
                CHECK(callbackCounts[Callbacks::speech_end_detected] == i + 1);
                CHECK(callbackCounts[Callbacks::no_match] == 0);
            }

            SPX_TRACE_VERBOSE("%s: Wait some more", __FUNCTION__);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            SPX_TRACE_VERBOSE("%s: Checking callback counts ...", __FUNCTION__);

            CHECK(callbackCounts[Callbacks::session_started] == numLoops);
            CHECK(callbackCounts[Callbacks::session_stopped] == numLoops);
            CHECK(callbackCounts[Callbacks::final_result] == numLoops);
            CHECK(callbackCounts[Callbacks::speech_start_detected] == numLoops);
            CHECK(callbackCounts[Callbacks::speech_end_detected] == numLoops);
            CHECK(callbackCounts[Callbacks::no_match] == 0);
        }

        UseMocks(false);
    }

    SECTION("Check that recognition result contains original json payload.")
    {
        SPX_TRACE_VERBOSE("%s: line=%d", __FUNCTION__, __LINE__);

        UseMocks(false);
        REQUIRE(exists(PAL::ToWString(input_file)));
        REQUIRE(!IsUsingMocks());

        auto recognizer = CreateRecognizers<SpeechRecognizer>(input_file);
        auto result = recognizer->RecognizeAsync().get();
        REQUIRE(!result->Properties.GetProperty(SpeechPropertyId::SpeechServiceResponse_Json).empty());
    }

    SECTION("Check that recognition result contains error details.")
    {
        SPX_TRACE_VERBOSE("%s: line=%d", __FUNCTION__, __LINE__);

        UseMocks(false);
        REQUIRE(exists(PAL::ToWString(input_file)));
        REQUIRE(!IsUsingMocks());
        auto badKeyConfig = SpeechConfig::FromSubscription("invalid_key", "invalid_region");
        auto audioConfig = AudioConfig::FromWavFileInput(input_file);
        auto recognizer = SpeechRecognizer::FromConfig(badKeyConfig, audioConfig);
        auto result = recognizer->RecognizeAsync().get();

        REQUIRE(result->Reason == ResultReason::Canceled);

        auto cancellation = CancellationDetails::FromResult(result);
        REQUIRE(cancellation->Reason == CancellationReason::Error);
        REQUIRE(!cancellation->ErrorDetails.empty());

        // NOTE: Looks like we still do need this...
        // TODO: there's a data race in the audio_pump thread when it tries to
        // pISpxAudioProcessor->SetFormat(nullptr); after exiting the loop.
        // Comment out the next line to see for yourself (repros on Linux build machines).
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
   
    SECTION("Wrong Key triggers Canceled Event ")
    {
        REQUIRE(exists(PAL::ToWString(input_file)));
        UseMocks(false);
        mutex mtx;
        condition_variable cv;

        bool connectionReportedError = false;
        string wrongKey = "wrongKey";

        auto sc = SpeechConfig::FromSubscription(wrongKey, "westus");
        auto a = AudioConfig::FromWavFileInput(input_file);
        auto recognizer = SpeechRecognizer::FromConfig(sc, a);

        recognizer->Canceled.Connect([&](const SpeechRecognitionCanceledEventArgs& args) {
            REQUIRE(args.Reason == CancellationReason::Error);
            REQUIRE(!args.ErrorDetails.empty());
            unique_lock<mutex> lock(mtx);
            connectionReportedError = true;
            cv.notify_one();
        });

        auto result = recognizer->RecognizeAsync().get();
        // TODO ENABLE AFTER FIXING BROKEN SERVICE       REQUIRE(result->Reason == ResultReason::Canceled);

        {
            unique_lock<mutex> lock(mtx);
            cv.wait_for(lock, std::chrono::seconds(10));
            // TODO ENABLE AFTER FIXING BROKEN SERVICE           REQUIRE(connectionReportedError);
        }
    }

    SECTION("German Speech Recognition works")
    {
        string german_input_file("tests/input/CallTheFirstOne.wav");
        REQUIRE(exists(PAL::ToWString(german_input_file)));

        auto sc = !Config::Endpoint.empty() ? SpeechConfig::FromEndpoint(Config::Endpoint, Keys::Speech) : SpeechConfig::FromSubscription(Keys::Speech, Config::Region);
        sc->SetSpeechRecognitionLanguage("de-DE");
        auto audioConfig = AudioConfig::FromWavFileInput(german_input_file);
        auto recognizer = SpeechRecognizer::FromConfig(sc, audioConfig);

        auto result = recognizer->RecognizeAsync().get();
        REQUIRE(result != nullptr);
        REQUIRE(!result->Text.empty());
    }

    SECTION("German Speech Recognition works")
    {
        string german_input_file("tests/input/CallTheFirstOne.wav");
        REQUIRE(exists(PAL::ToWString(german_input_file)));

        auto sc = !Config::Endpoint.empty() ? SpeechConfig::FromEndpoint(Config::Endpoint, Keys::Speech) : SpeechConfig::FromSubscription(Keys::Speech, Config::Region);
        sc->SetSpeechRecognitionLanguage("de-DE");
        auto audioConfig = AudioConfig::FromWavFileInput(german_input_file);
        auto recognizer = SpeechRecognizer::FromConfig(sc, audioConfig);

        auto result = recognizer->RecognizeAsync().get();
        REQUIRE(result != nullptr);
        REQUIRE(!result->Text.empty());
    }
}

TEST_CASE("KWS basics", "[api][cxx]")
{
    SPX_TRACE_SCOPE(__FUNCTION__, __FUNCTION__);
    GIVEN("Mocks for USP, KWS, and the Microphone...")
    {
        SPX_TRACE_VERBOSE("%s: line=%d", __FUNCTION__, __LINE__);

        UseMocks(true);

        mutex mtx;
        condition_variable cv;

        int gotFinalResult = 0;
        int gotSessionStopped = 0;

        WHEN("We do a keyword recognition with a speech recognizer")
        {
            SPX_TRACE_VERBOSE("%s: line=%d", __FUNCTION__, __LINE__);

            auto recognizer = SpeechRecognizer::FromConfig(CurrentSpeechConfig());
            REQUIRE(recognizer != nullptr);
            REQUIRE(IsUsingMocks(true));

            recognizer->FinalResult += [&](const SpeechRecognitionEventArgs& /* e */) {
                std::unique_lock<std::mutex> lock(mtx);
                gotFinalResult++;
                SPX_TRACE_VERBOSE("gotFinalResult=%d", gotFinalResult);
            };

            recognizer->SessionStopped += [&](const SessionEventArgs& /* e */) {
                std::unique_lock<std::mutex> lock(mtx);
                gotSessionStopped++;
                SPX_TRACE_VERBOSE("gotSessionStopped=%d", gotSessionStopped);
                cv.notify_all();
            };

            auto model = KeywordRecognitionModel::FromFile("tests/input/heycortana_en-US.table");
            recognizer->StartKeywordRecognitionAsync(model);

            THEN("We wait up to 30 seconds for a KwsSingleShot recognition and it's accompanying SessionStopped")
            {
                SPX_TRACE_VERBOSE("%s: line=%d", __FUNCTION__, __LINE__);

                std::unique_lock<std::mutex> lock(mtx);
                cv.wait_for(lock, std::chrono::seconds(30), [&] { return gotFinalResult >= 1 && gotSessionStopped >= 1; });
                lock.unlock();

                recognizer->StopKeywordRecognitionAsync().get();

                THEN("We should see that we got at least 1 FinalResult and the same number of SessionStopped events")
                {
                    REQUIRE(gotFinalResult >= 1);
                }
            }

            // std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        UseMocks(false);
    }
}

TEST_CASE("Speech on local server", "[api][cxx]")
{
    SPX_TRACE_SCOPE(__FUNCTION__, __FUNCTION__);
    SECTION("Stress testing against the local server")
    {
        SPX_TRACE_VERBOSE("%s: line=%d", __FUNCTION__, __LINE__);

        if (Config::Endpoint.empty())
        {
            return;
        }

        UseMocks(false);
        REQUIRE(exists(PAL::ToWString(input_file)));
        REQUIRE(!IsUsingMocks());

        const int numLoops = 10;
        
        auto sc = SpeechConfig::FromEndpoint(Config::Endpoint, R"({"max_timeout":"0"})");
        for (int i = 0; i < numLoops; i++)
        {
            auto audioConfig = AudioConfig::FromWavFileInput(input_file);
            auto recognizer = SpeechRecognizer::FromConfig(sc, audioConfig);
            auto result = recognizer->RecognizeAsync().get();
            REQUIRE(result->Reason == ResultReason::RecognizedSpeech);
            REQUIRE(result->Text == "Remind me to buy 5 iPhones.");
        }

        // BUGBUG: this currently fails because CSpxAudioStreamSession::WaitForRecognition() returns a nullptr on a timeout.
        /*
        factory = SpeechFactory::FromEndpoint(PAL::ToWString(Config::Endpoint), LR"({"max_timeout":"10000"})");
        for (int i = 0; i < numLoops; i++)
        {
        auto audioConfig = AudioConfig::FromWavFileInput(input_file);
        auto recognizer = factory->CreateSpeechRecognizerFromConfig(audioConfig);
        auto result = recognizer->RecognizeAsync().get();
        REQUIRE(result->Reason == ResultReason::RecognizedSpeech);
        REQUIRE(result->Text == L"Remind me to buy 5 iPhones.");
        }
        */
    }
}

TEST_CASE("Speech Recognizer is thread-safe.", "[api][cxx]")
{
    SPX_TRACE_SCOPE(__FUNCTION__, __FUNCTION__);

    REQUIRE(exists(PAL::ToWString(input_file)));

    mutex mtx;
    condition_variable cv;

    SECTION("Check for competing or conflicting conditions in destructor.")
    {
        SPX_TRACE_VERBOSE("%s: line=%d", __FUNCTION__, __LINE__);

        bool callback_invoked = false;

        REQUIRE(!IsUsingMocks());
        auto recognizer = CreateRecognizers<SpeechRecognizer>(input_file);

        auto callback = [&](const SpeechRecognitionEventArgs& args)
        {
            {
                UNUSED(args);
                unique_lock<mutex> lock(mtx);
                callback_invoked = true;
                cv.notify_one();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        };

        auto canceledCallback = [&](const SpeechRecognitionCanceledEventArgs& args) { callback(args); };

        recognizer->FinalResult.Connect(callback);
        recognizer->Canceled.Connect(canceledCallback); // Canceled is called if there are connection issues.

        auto result = recognizer->RecognizeAsync().get();

        {
            unique_lock<mutex> lock(mtx);
            cv.wait_for(lock, std::chrono::seconds(10), [&] { return callback_invoked; });
            REQUIRE(callback_invoked);
        }
        recognizer.reset();
    }

    SECTION("Check for a deadlock in disconnect.")
    {
        SPX_TRACE_VERBOSE("%s: line=%d", __FUNCTION__, __LINE__);

        REQUIRE(!IsUsingMocks());
        auto recognizer = CreateRecognizers<SpeechRecognizer>(input_file);

        auto callback1 = [&](const SpeechRecognitionEventArgs& args)
        {
            if (args.Result->Reason == ResultReason::RecognizedSpeech) 
            {
                recognizer->Canceled.DisconnectAll();
            }
            else
            {
                recognizer->FinalResult.DisconnectAll();
            }
        };
        auto canceledCallback1 = [&](const SpeechRecognitionCanceledEventArgs& args) { callback1(args); };

        recognizer->FinalResult.Connect(callback1);
        recognizer->Canceled.Connect(canceledCallback1);

        auto result = recognizer->RecognizeAsync().get();
        UNUSED(result);

        auto callback2 = [&](const SpeechRecognitionEventArgs&)
        {
            recognizer->Canceled.DisconnectAll();
            recognizer->FinalResult.DisconnectAll();
        };
        auto canceledCallback2 = [&](const SpeechRecognitionCanceledEventArgs& args) { callback2(args); };

        recognizer = CreateRecognizers<SpeechRecognizer>(input_file);
        recognizer->FinalResult.Connect(callback2);
        recognizer->Canceled.Connect(canceledCallback2);

        result = recognizer->RecognizeAsync().get();
        UNUSED(result);

        auto callback3 = [&](const SpeechRecognitionEventArgs&)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            recognizer.reset();
        };
        auto canceledCallback3 = [&](const SpeechRecognitionEventArgs& args) { callback3(args); };
        
        recognizer = CreateRecognizers<SpeechRecognizer>(input_file);
        recognizer->FinalResult.Connect(callback3);
        recognizer->Canceled.Connect(canceledCallback3);
        auto future = recognizer->RecognizeAsync();
        UNUSED(future);
    }
}
