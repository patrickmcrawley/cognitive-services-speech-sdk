//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.md file in the project root for full license information.
//

using System;
using System.Threading.Tasks;

namespace Microsoft.CognitiveServices.Speech.Translation
{
    /// <summary>
    /// Performs translation on the speech input.
    /// </summary>
    /// <example>
    /// An example to use the translation recognizer from microphone and listen to events generated by the recognizer.
    /// <code>
    /// public async Task TranslationContinuousRecognitionAsync()
    /// {
    ///     // Creates an instance of a speech translator config with specified subscription key and service region. 
    ///     // Replace with your own subscription key and service region (e.g., "westus").
    ///     var config = SpeechTranslatorConfig.FromSubscription("YourSubscriptionKey", "YourServiceRegion");
    ///
    ///     // Sets source and target languages.
    ///     string fromLanguage = "en-US";
    ///     config.Language = fromLanguage;
    ///     config.AddTargetLanguage("de");
    ///
    ///     // Sets voice name of synthesis output.
    ///     const string GermanVoice = "de-DE-Hedda";
    ///     config.VoiceName = GermanVoice;
    ///     // Creates a translation recognizer using microphone as audio input.
    ///     using (var recognizer = new TranslationRecognizer(config))
    ///     {
    ///         // Subscribes to events.
    ///         recognizer.IntermediateResultReceived += (s, e) =>
    ///         {
    ///             Console.WriteLine($"RECOGNIZING in '{fromLanguage}': Text={e.Result.Text}");
    ///             foreach (var element in e.Result.Translations)
    ///             {
    ///                 Console.WriteLine($"    TRANSLATING into '{element.Key}': {element.Value}");
    ///             }
    ///         };
    ///
    ///         recognizer.FinalResultReceived += (s, e) =>
    ///         {
    ///             if (result.Reason == ResultReason.TranslatedSpeech)
    ///             {
    ///                 Console.WriteLine($"\nFinal result: Reason: {e.Result.Reason.ToString()}, recognized text in {fromLanguage}: {e.Result.Text}.");
    ///                 foreach (var element in e.Result.Translations)
    ///                 {
    ///                     Console.WriteLine($"    TRANSLATING into '{element.Key}': {element.Value}");
    ///                 }
    ///             }
    ///         };
    ///
    ///         recognizer.SynthesisResultReceived += (s, e) =>
    ///         {
    ///             Console.WriteLine(e.Result.Audio.Length != 0
    ///                 ? $"AudioSize: {e.Result.Audio.Length}"
    ///                 : $"AudioSize: {e.Result.Audio.Length} (end of synthesis data)");
    ///         };
    ///
    ///         recognizer.Canceled += (s, e) =>
    ///         {
    ///             Console.WriteLine($"\nRecognition canceled. Reason: {e.Reason}; ErrorDetails: {e.ErrorDetails}");
    ///         };
    ///
    ///         recognizer.OnSessionEvent += (s, e) =>
    ///         {
    ///             Console.WriteLine($"\nSession event. Event: {e.EventType.ToString()}.");
    ///         };
    ///
    ///         // Starts continuous recognition. Uses StopContinuousRecognitionAsync() to stop recognition.
    ///         Console.WriteLine("Say something...");
    ///         await recognizer.StartContinuousRecognitionAsync().ConfigureAwait(false);
    ///
    ///         do
    ///         {
    ///             Console.WriteLine("Press Enter to stop");
    ///         } while (Console.ReadKey().Key != ConsoleKey.Enter);
    ///
    ///         // Stops continuous recognition.
    ///         await recognizer.StopContinuousRecognitionAsync().ConfigureAwait(false);
    ///     }
    /// }
    /// </code>
    /// </example>
    public sealed class TranslationRecognizer : Recognizer
    {
        /// <summary>
        /// The event <see cref="IntermediateResultReceived"/> signals that an intermediate recognition result is received.
        /// </summary>
        public event EventHandler<TranslationTextResultEventArgs> IntermediateResultReceived;

        /// <summary>
        /// The event <see cref="FinalResultReceived"/> signals that a final recognition result is received.
        /// </summary>
        public event EventHandler<TranslationTextResultEventArgs> FinalResultReceived;

        /// <summary>
        /// The event <see cref="Canceled"/> signals that the speech to text/synthesis translation was canceled.
        /// </summary>
        public event EventHandler<TranslationTextResultCanceledEventArgs> Canceled;

        /// <summary>
        /// The event <see cref="SynthesisResultReceived"/> signals that a translation synthesis result is received.
        /// </summary>
        public event EventHandler<TranslationSynthesisResultEventArgs> SynthesisResultReceived;

        /// <summary>
        /// Creates a translation recognizer using the default microphone input for a specified translator configuration.
        /// </summary>
        /// <param name="config">Translator config.</param>
        /// <returns>A translation recognizer instance.</returns>
        public TranslationRecognizer(SpeechTranslatorConfig config)
            : this(config != null ? config.impl : throw new ArgumentNullException(nameof(config)), null)
        {
        }

        /// <summary>
        /// Creates a translation recognizer using the specified speech translator and audio configuration.
        /// </summary>
        /// <param name="config">Translator config.</param>
        /// <param name="audioConfig">Audio config.</param>
        /// <returns>A translation recognizer instance.</returns>
        public TranslationRecognizer(SpeechTranslatorConfig config, Audio.AudioConfig audioConfig)
            : this(config != null ? config.impl : throw new ArgumentNullException(nameof(config)),
                   audioConfig != null ? audioConfig.configImpl : throw new ArgumentNullException(nameof(audioConfig)))
        {
            this.audioConfig = audioConfig;
        }

        internal TranslationRecognizer(Internal.SpeechTranslatorConfig config, Internal.AudioConfig audioConfig)
        {
            this.recoImpl = Internal.TranslationRecognizer.FromConfig(config, audioConfig);

            intermediateResultHandler = new ResultHandlerImpl(this, isFinalResultHandler: false);
            recoImpl.IntermediateResult.Connect(intermediateResultHandler);

            finalResultHandler = new ResultHandlerImpl(this, isFinalResultHandler: true);
            recoImpl.FinalResult.Connect(finalResultHandler);

            synthesisResultHandler = new SynthesisHandlerImpl(this);
            recoImpl.TranslationSynthesisResultEvent.Connect(synthesisResultHandler);

            canceledHandler = new CanceledHandlerImpl(this);
            recoImpl.Canceled.Connect(canceledHandler);

            recoImpl.SessionStarted.Connect(sessionStartedHandler);
            recoImpl.SessionStopped.Connect(sessionStoppedHandler);
            recoImpl.SpeechStartDetected.Connect(speechStartDetectedHandler);
            recoImpl.SpeechEndDetected.Connect(speechEndDetectedHandler);

            Parameters = new PropertyCollectionImpl(recoImpl.Parameters);
        }

        /// <summary>
        /// Gets the language name that was set when the recognizer was created.
        /// </summary>
        public string SpeechRecognitionLanguage
        {
            get
            {
                return Parameters.Get(SpeechPropertyId.SpeechServiceConnection_TranslationFromLanguage);
            }
        }

        /// <summary>
        /// Gets target languages for translation that were set when the recognizer was created.
        /// The language is specified in BCP-47 format. The translation will provide translated text for each of language.
        /// </summary>
        public string[] TargetLanguages
        {
            get
            {
                var plainStr = Parameters.Get(SpeechPropertyId.SpeechServiceConnection_TranslationToLanguages);
                return plainStr.Split(',');
            }
        }

        /// <summary>
        /// Gets the name of output voice if speech synthesis is used.
        /// </summary>
        public string VoiceName
        {
            get
            {
                return Parameters.Get(SpeechPropertyId.SpeechServiceConnection_TranslationVoice);
            }
        }

        /// <summary>
        /// The collection of parameters and their values defined for this <see cref="TranslationRecognizer"/>.
        /// </summary>
        public IPropertyCollection Parameters { get; internal set; }

        /// <summary>
        /// Gets/sets authorization token used to communicate with the service.
        /// </summary>
        public string AuthorizationToken
        {
            get
            {
                return this.recoImpl.GetAuthorizationToken();
            }

            set
            {
                if (value == null)
                {
                    throw new ArgumentNullException(nameof(value));
                }

                this.recoImpl.SetAuthorizationToken(value);
            }
        }

        /// <summary>
        /// Starts recognition and translation, and stops after the first utterance is recognized. The task returns the translation text as result.
        /// Note: RecognizeAsync() returns when the first utterance has been recognized, so it is suitable only for single shot recognition like command or query. For long-running recognition, use StartContinuousRecognitionAsync() instead.
        /// </summary>
        /// <returns>A task representing the recognition operation. The task returns a value of <see cref="TranslationTextResult"/> </returns>
        /// <example>
        /// Create a translation recognizer, get and print the recognition result
        /// <code>
        /// public async Task TranslationSingleShotRecognitionAsync()
        /// {
        ///     // Creates an instance of a speech translator config with specified subscription key and service region. 
        ///     // Replace with your own subscription key and service region (e.g., "westus").
        ///     var config = SpeechTranslatorConfig.FromSubscription("YourSubscriptionKey", "YourServiceRegion");
        ///
        ///     string fromLanguage = "en-US";
        ///     config.Language = fromLanguage;
        ///     config.AddTargetLanguage("de");
        ///
        ///     // Creates a translation recognizer.
        ///     using (var recognizer = new TranslationRecognizer(config))
        ///     {
        ///         // Starts recognizing.
        ///         Console.WriteLine("Say something...");
        ///
        ///         // Performs recognition. RecognizeAsync() returns when the first utterance has been recognized,
        ///         // so it is suitable only for single shot recognition like command or query. For long-running
        ///         // recognition, use StartContinuousRecognitionAsync() instead.
        ///         var result = await recognizer.RecognizeAsync();
        ///
        ///         if (result.Reason == ResultReason.TranslatedSpeech)
        ///         {
        ///             Console.WriteLine($"\nFinal result: Reason: {result.Reason.ToString()}, recognized text: {result.Text}.");
        ///             foreach (var element in result.Translations)
        ///             {
        ///                 Console.WriteLine($"    TRANSLATING into '{element.Key}': {element.Value}");
        ///             }
        ///         }
        ///     }
        /// }
        /// </code>
        /// </example>
        public Task<TranslationTextResult> RecognizeAsync()
        {
            return Task.Run(() => { return new TranslationTextResult(this.recoImpl.Recognize()); });
        }

        /// <summary>
        /// Starts recognition and translation on a continous audio stream, until StopContinuousRecognitionAsync() is called.
        /// User must subscribe to events to receive translation results.
        /// </summary>
        /// <returns>A task representing the asynchronous operation that starts the recognition.</returns>
        public Task StartContinuousRecognitionAsync()
        {
            return Task.Run(() => { this.recoImpl.StartContinuousRecognition(); });
        }

        /// <summary>
        /// Stops continuous recognition and translation.
        /// </summary>
        /// <returns>A task representing the asynchronous operation that stops the translation.</returns>
        public Task StopContinuousRecognitionAsync()
        {
            return Task.Run(() => { this.recoImpl.StopContinuousRecognition(); });
        }

        /// <summary>
        /// Starts speech recognition on a continuous audio stream with keyword spotting, until StopKeywordRecognitionAsync() is called.
        /// User must subscribe to events to receive recognition results.
        /// Note: Keyword spotting functionality is only available on the Cognitive Services Device SDK. This functionality is currently not included in the SDK itself.
        /// </summary>
        /// <param name="model">The keyword recognition model that specifies the keyword to be recognized.</param>
        /// <returns>A task representing the asynchronous operation that starts the recognition.</returns>
        public Task StartKeywordRecognitionAsync(KeywordRecognitionModel model)
        {
            return Task.Run(() => { this.recoImpl.StartKeywordRecognition(model.modelImpl); });
        }

        /// <summary>
        /// Stops continuous speech recognition with keyword spotting.
        /// Note: Key word spotting functionality is only available on the Cognitive Services Device SDK. This functionality is currently not included in the SDK itself.
        /// </summary>
        /// <returns>A task representing the asynchronous operation that stops the recognition.</returns>
        public Task StopKeywordRecognitionAsync()
        {
            return Task.Run(() => { this.recoImpl.StopKeywordRecognition(); });
        }

        protected override void Dispose(bool disposing)
        {
            if (disposed)
            {
                return;
            }

            if (disposing)
            {
                recoImpl?.Dispose();

                intermediateResultHandler?.Dispose();
                finalResultHandler?.Dispose();
                canceledHandler?.Dispose();

                disposed = true;
                base.Dispose(disposing);
            }
        }

        internal readonly Internal.TranslationRecognizer recoImpl;
        private readonly ResultHandlerImpl intermediateResultHandler;
        private readonly ResultHandlerImpl finalResultHandler;
        private readonly SynthesisHandlerImpl synthesisResultHandler;
        private readonly CanceledHandlerImpl canceledHandler;
        private bool disposed = false;
        private readonly Audio.AudioConfig audioConfig;

        // Defines an internal class to raise a C# event for intermediate/final result when a corresponding callback is invoked by the native layer.
        private class ResultHandlerImpl : Internal.TranslationTextEventListener
        {
            public ResultHandlerImpl(TranslationRecognizer recognizer, bool isFinalResultHandler)
            {
                this.recognizer = recognizer;
                this.isFinalResultHandler = isFinalResultHandler;
            }

            public override void Execute(Internal.TranslationTextResultEventArgs eventArgs)
            {
                if (recognizer.disposed)
                {
                    return;
                }

                TranslationTextResultEventArgs resultEventArg = new TranslationTextResultEventArgs(eventArgs);
                var handler = isFinalResultHandler ? recognizer.FinalResultReceived : recognizer.IntermediateResultReceived;
                if (handler != null)
                {
                    handler(this.recognizer, resultEventArg);
                }
            }

            private TranslationRecognizer recognizer;
            private bool isFinalResultHandler;
        }

        // Defines an internal class to raise a C# event for error during recognition when a corresponding callback is invoked by the native layer.
        private class CanceledHandlerImpl : Internal.TranslationTextCanceledEventListener
        {
            public CanceledHandlerImpl(TranslationRecognizer recognizer)
            {
                this.recognizer = recognizer;
            }

            public override void Execute(Microsoft.CognitiveServices.Speech.Internal.TranslationTextResultCanceledEventArgs eventArgs)
            {
                if (recognizer.disposed)
                {
                    return;
                }

                var canceledEventArgs = new TranslationTextResultCanceledEventArgs(eventArgs);
                var handler = this.recognizer.Canceled;

                if (handler != null)
                {
                    handler(this.recognizer, canceledEventArgs);
                }
            }

            private TranslationRecognizer recognizer;
        }

        // Defines an internal class to raise a C# event for intermediate/final result when a corresponding callback is invoked by the native layer.
        private class SynthesisHandlerImpl : Internal.TranslationSynthesisEventListener
        {
            public SynthesisHandlerImpl(TranslationRecognizer recognizer)
            {
                this.recognizer = recognizer;
            }

            public override void Execute(Internal.TranslationSynthesisResultEventArgs eventArgs)
            {
                if (recognizer.disposed)
                {
                    return;
                }

                var resultEventArg = new TranslationSynthesisResultEventArgs(eventArgs);
                var handler = recognizer.SynthesisResultReceived;
                if (handler != null)
                {
                    handler(this.recognizer, resultEventArg);
                }
            }

            private TranslationRecognizer recognizer;
        }
    }

}
