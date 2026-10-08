long long
compute_score()
{
    const long long score =
        baseScoreWithLongDescriptiveName
        + std::abs(measuredValueWithLongDescriptiveName - targetValueWithLongDescriptiveName)
        * errorWeightWithLongDescriptiveName
        / std::max(
            normalizationScaleWithAnExtremelyLongDescriptiveNameThatForcesFunctionExpansion,
            minimumScaleWithAnExtremelyLongDescriptiveNameThatForcesFunctionExpansion
            )
        - std::gcd(sampleCountWithLongDescriptiveName, bucketCountWithLongDescriptiveName)
        * commonFactorPenaltyWithAnExtremelyLongDescriptiveNameThatForcesExpansion
        +
        (
            sampleIndexWithAnExtremelyLongDescriptiveNameThatForcesMultiplicativeExpansion
            * strideWithAnExtremelyLongDescriptiveNameThatForcesMultiplicativeExpansion
            + channelOffsetWithAnExtremelyLongDescriptiveNameThatForcesAdditiveExpansion
        )
        % veryLongRingSizeWithLongDescriptiveName;

    return score;
}
