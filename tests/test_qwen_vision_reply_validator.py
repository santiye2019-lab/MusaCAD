import unittest
from scripts.validate_qwen_vision_result import (
    extract_answer, normalize, validate_synthetic_vision
)


class QwenVisionReplyValidatorTest(unittest.TestCase):
    def test_turkish_dotless_i_is_not_combining_mark(self):
        self.assertEqual(normalize("Kırmızı"), "kirmizi")
        self.assertEqual(normalize("KIRMIZI ve MAVİ"), "kirmizi ve mavi")

    def test_realistic_openai_style_turkish_answer(self):
        sample = {"choices": [{"message": {"content": "Solda kırmızı, sağda mavi."}}]}
        self.assertEqual(validate_synthetic_vision(sample), (True, "PASS"))

    def test_cloudflare_wrapped_english_answer(self):
        sample = {"result": {"choices": [
            {"message": {"content": [{"type": "text", "text": "Red and Blue"}]}}
        ]}}
        self.assertEqual(validate_synthetic_vision(sample), (True, "PASS"))

    def test_nonvisual_and_empty_responses_fail_closed(self):
        self.assertEqual(validate_synthetic_vision({"choices": [{"message": {
            "content": "Görseli okuyamıyorum."}}]})[0], False)
        self.assertEqual(validate_synthetic_vision({"choices": [
            {"message": {"content": None}}]}), (False, "EMPTY_CONTENT"))
        self.assertEqual(validate_synthetic_vision({}), (False, "EMPTY_CONTENT"))

    def test_both_color_words_required(self):
        self.assertFalse(validate_synthetic_vision({
            "choices": [{"message": {"content": "Bence kırmızı."}}]})[0])
        self.assertFalse(validate_synthetic_vision({
            "choices": [{"message": {"content": "Blue."}}]})[0])


if __name__ == "__main__":
    unittest.main()
