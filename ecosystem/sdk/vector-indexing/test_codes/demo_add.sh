#!/bin/bash

echo "=== Adding 10 demo texts to ResilientDB ==="

texts=(
"Large language models can generate human-like text and assist with tasks such as summarization, translation, and code generation."
"Photosynthesis allows plants to convert sunlight into chemical energy, producing oxygen as a byproduct."
"Kyoto is known for its ancient temples, traditional wooden houses, and beautiful seasonal landscapes."
"Strong branding helps companies build customer trust and differentiate themselves in competitive markets."
"Regular exercise improves cardiovascular health, increases muscle strength, and reduces stress levels."
"Active learning encourages students to participate, discuss ideas, and apply knowledge rather than passively listen."
"Sourdough bread develops its unique flavor through natural fermentation using wild yeast and lactic acid bacteria."
"Reducing plastic waste requires better recycling systems and increased use of biodegradable materials."
"Impressionist painters focused on capturing light and movement rather than creating precise, realistic details."
"Basketball requires teamwork, quick decision-making, and precise coordination between players on the court."
)

for text in "${texts[@]}"
do
    echo "→ Adding:"
    echo "   \"$text\""
    python3 kv_vector.py --add "$text"
    echo ""
done

echo "=== Done: All demo texts added ==="
