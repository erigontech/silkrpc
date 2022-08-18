/*
   Copyright 2022 The Silkrpc Authors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <gmock/gmock.h>

#include <string>

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);

    auto& listeners = ::testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());

    class CatchListener : public ::testing::EmptyTestEventListener {
      public:
        void OnTestPartResult(const ::testing::TestPartResult& result) override {
            if (!result.failed()) {
                return;
            }
            const auto file = result.file_name() ? result.file_name() : "unknown";
            const auto line = result.line_number() != -1 ? result.line_number() : 0;
            const auto message = result.message() ? result.message() : "no message";
            ::Catch::SourceLineInfo source_line_info(file, static_cast<std::size_t>(line));
            auto result_disposition = result.fatally_failed() ? ::Catch::ResultDisposition::Normal
                                                              : ::Catch::ResultDisposition::ContinueOnFailure;
            ::Catch::AssertionHandler assertion("GTEST", source_line_info, "", result_disposition);
            assertion.handleMessage(::Catch::ResultWas::ExplicitFailure, result.message());
            assertion.setCompleted();
        }
    };
    listeners.Append(new CatchListener);

    return Catch::Session().run(argc, argv);
}
