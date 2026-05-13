#ifndef FEATUREEXTRACTOR_H
#define FEATUREEXTRACTOR_H

#include "sessionfeaturevector.h"
#include "sessionsummary.h"
#include "typingsession.h"

namespace FeatureExtractor
{
SessionSummary buildSessionSummary(const TypingSession &session);

SessionFeatureVector buildFeatureVector(const TypingSession &session,
                                        const SessionSummary &summary);
}

#endif // FEATUREEXTRACTOR_H
