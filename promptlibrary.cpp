#include "promptlibrary.h"

QVector<PromptDefinition> PromptLibrary::fixedTextPrompts()
{
    return {
        {
            QStringLiteral("prompt_01"),
            QStringLiteral("Pangram sample"),
            QStringLiteral("The quick brown fox jumps over the lazy dog. Secure typing sample 2026.")
        },
        {
            QStringLiteral("prompt_02"),
            QStringLiteral("Cybersecurity sample"),
            QStringLiteral("Cybersecurity depends on accurate identity checks, careful logging, and trusted user behavior.")
        },
        {
            QStringLiteral("prompt_03"),
            QStringLiteral("Verification sample"),
            QStringLiteral("Please type this fixed verification text exactly as shown, including punctuation and spaces.")
        }
    };
}

PromptDefinition PromptLibrary::promptByLabel(const QString &label)
{
    const QVector<PromptDefinition> prompts = fixedTextPrompts();

    for (const PromptDefinition &prompt : prompts)
    {
        if (prompt.label == label)
        {
            return prompt;
        }
    }

    return {};
}