//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.md file in the project root for full license information.
//

using System;
using System.Diagnostics.Contracts;
using System.Globalization;

namespace Microsoft.CognitiveServices.Speech
{
    /// <summary>
    /// Contains detailed information about result of a recognition operation.
    /// </summary>
    public class RecognitionResult
    {
        internal RecognitionResult(Internal.RecognitionResult result)
        {
            Contract.Requires((int)ResultReason.NoMatch == (int)Internal.ResultReason.NoMatch);
            Contract.Requires((int)ResultReason.Canceled == (int)Internal.ResultReason.Canceled);
            Contract.Requires((int)ResultReason.RecognizingSpeech == (int)Internal.ResultReason.RecognizingSpeech);
            Contract.Requires((int)ResultReason.RecognizedSpeech == (int)Internal.ResultReason.RecognizedSpeech);
            Contract.Requires((int)ResultReason.RecognizingIntent == (int)Internal.ResultReason.RecognizingIntent);
            Contract.Requires((int)ResultReason.RecognizedIntent == (int)Internal.ResultReason.RecognizedIntent);
            Contract.Requires((int)ResultReason.TranslatingSpeech == (int)Internal.ResultReason.TranslatingSpeech);
            Contract.Requires((int)ResultReason.TranslatedSpeech == (int)Internal.ResultReason.TranslatedSpeech);
            Contract.Requires((int)ResultReason.SynthesizingAudio == (int)Internal.ResultReason.SynthesizingAudio);
            Contract.Requires((int)ResultReason.SynthesizingAudioCompleted == (int)Internal.ResultReason.SynthesizingAudioCompleted);

            resultImpl = result;
            this.ResultId = result.ResultId;
            this.Text = result.Text;
            this.Reason = (ResultReason)((int)result.Reason);
            Properties = new PropertyCollection(result.Properties);
        }

        /// <summary>
        /// Specifies the result identifier.
        /// </summary>
        public string ResultId { get; }

        /// <summary>
        /// Specifies status of speech recognition result.
        /// </summary>
        public ResultReason Reason { get; }

        /// <summary>
        /// Presents the recognized text in the result.
        /// </summary>
        public string Text { get; }

        /// <summary>
        /// Duration of the recognized speech.
        /// </summary>
        public TimeSpan Duration => TimeSpan.FromTicks((long)this.resultImpl.Duration());

        /// <summary>
        /// Offset of the recognized speech in ticks. A single tick represents one hundred nanoseconds or one ten-millionth of a second.
        /// </summary>
        public long OffsetInTicks => (long)this.resultImpl.Offset();

        /// <summary>
        /// Contains properties of the results.
        /// </summary>
        public PropertyCollection Properties { get; private set; }

        /// <summary>
        /// Returns a string that represents the speech recognition result.
        /// </summary>
        /// <returns>A string that represents the speech recognition result.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture,"ResultId:{0} Reason:{1} Recognized text:<{2}>. Json:{3}", 
                ResultId, Reason, Text, Properties.GetProperty(PropertyId.SpeechServiceResponse_JsonResult));
        }

        // Hold the reference.
        internal Internal.RecognitionResult resultImpl { get; }
    }

    /// <summary>
    /// Contains detailed information about why a result was canceled.
    /// </summary>
    public class CancellationDetails
    {
        /// <summary>
        /// Creates an instance of CancellationDetails object for the canceled SpeechRecognitionResult.
        /// </summary>
        /// <param name="result">The result that was canceled.</param>
        /// <returns>The CancellationDetails object being created.</returns>
        public static CancellationDetails FromResult(RecognitionResult result)
        {
            var canceledImpl = Internal.CancellationDetails.FromResult(result.resultImpl);
            return new CancellationDetails(canceledImpl);
        }

        internal CancellationDetails(Internal.CancellationDetails cancellation)
        {
            Contract.Requires((int)CancellationReason.Error == (int)Internal.CancellationReason.Error);
            Contract.Requires((int)CancellationReason.EndOfStream == (int)Internal.CancellationReason.EndOfStream);

            Contract.Requires((int)CancellationErrorCode.NoError == (int)Internal.CancellationErrorCode.NoError);
            Contract.Requires((int)CancellationErrorCode.AuthenticationFailure == (int)Internal.CancellationErrorCode.AuthenticationFailure);
            Contract.Requires((int)CancellationErrorCode.BadRequestParameters== (int)Internal.CancellationErrorCode.BadRequestParameters);
            Contract.Requires((int)CancellationErrorCode.TooManyRequests == (int)Internal.CancellationErrorCode.TooManyRequests);
            Contract.Requires((int)CancellationErrorCode.ConnectionFailure == (int)Internal.CancellationErrorCode.ConnectionFailure);
            Contract.Requires((int)CancellationErrorCode.ServiceTimeout == (int)Internal.CancellationErrorCode.ServiceTimeout);
            Contract.Requires((int)CancellationErrorCode.ServiceError == (int)Internal.CancellationErrorCode.ServiceError);
            Contract.Requires((int)CancellationErrorCode.RuntimeError == (int)Internal.CancellationErrorCode.RuntimeError);

            canceledImpl = cancellation;
            this.Reason = (CancellationReason)((int)cancellation.Reason);
            this.ErrorCode = (CancellationErrorCode)((int)cancellation.ErrorCode);
            this.ErrorDetails = cancellation.ErrorDetails;
        }

        /// <summary>
        /// The reason the recognition was canceled.
        /// </summary>
        public CancellationReason Reason { get; private set; }

        /// <summary>
        /// The error code in case of an unsuccessful recognition.
        /// Added in version 1.1.0.
        /// </summary>
        public CancellationErrorCode ErrorCode { get; private set; }

        /// <summary>
        /// The error message In case of an unsuccessful recognition.
        /// This field is only filled-out if the reason canceled (<see cref="Reason"/>) is set to Error.
        /// </summary>
        public string ErrorDetails { get; private set; }

        /// <summary>
        /// Returns a string that represents the cancellation details.
        /// </summary>
        /// <returns>A string that represents the cancellation details.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture,"Reason:{0} ErrorDetails:<{1}>", Reason, ErrorDetails);
        }

        // Hold the reference.
        private Internal.CancellationDetails canceledImpl;
    }

    /// <summary>
    /// Contains detailed information for NoMatch recognition results.
    /// </summary>
    public class NoMatchDetails
    {
        /// <summary>
        /// Creates an instance of NoMatchDetails object for NoMatch RecognitionResults.
        /// </summary>
        /// <param name="result">The recognition result that was not recognized.</param>
        /// <returns>The NoMatchDetails object being created.</returns>
        public static NoMatchDetails FromResult(RecognitionResult result)
        {
            var noMatchImpl = Internal.NoMatchDetails.FromResult(result.resultImpl);
            return new NoMatchDetails(noMatchImpl);
        }

        internal NoMatchDetails(Internal.NoMatchDetails noMatch)
        {
            Contract.Requires((int)NoMatchReason.NotRecognized == (int)Internal.NoMatchReason.NotRecognized);
            Contract.Requires((int)NoMatchReason.InitialSilenceTimeout == (int)Internal.NoMatchReason.InitialSilenceTimeout);
            Contract.Requires((int)NoMatchReason.InitialBabbleTimeout == (int)Internal.NoMatchReason.InitialBabbleTimeout);

            noMatchImpl = noMatch;
            this.Reason = (NoMatchReason)((int)noMatch.Reason);
        }

        /// <summary>
        /// The reason the result was not recognized.
        /// </summary>
        public NoMatchReason Reason { get; private set; }

        /// <summary>
        /// Returns a string that represents the cancellation details.
        /// </summary>
        /// <returns>A string that represents the cancellation details.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "NoMatchReason:{0}", Reason);
        }

        // Hold the reference.
        private Internal.NoMatchDetails noMatchImpl;
    }
}