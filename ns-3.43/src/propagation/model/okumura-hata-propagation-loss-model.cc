/*
 * Copyright (c) 2011, 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo  <marco.miozzo@cttc.es>,
 *         Nicola Baldo <nbaldo@cttc.es>
 *
 */
#include "okumura-hata-propagation-loss-model.h"

#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"

#include <algorithm>
#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OkumuraHataPropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED(OkumuraHataPropagationLossModel);

TypeId
OkumuraHataPropagationLossModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::OkumuraHataPropagationLossModel")
            .SetParent<PropagationLossModel>()
            .SetGroupName("Propagation")
            .AddConstructor<OkumuraHataPropagationLossModel>()
            .AddAttribute("Frequency",
                          "The propagation frequency in Hz",
                          DoubleValue(2160e6),
                          MakeDoubleAccessor(&OkumuraHataPropagationLossModel::m_frequency),
                          MakeDoubleChecker<double>())
            .AddAttribute(
                "Environment",
                "Environment Scenario",
                EnumValue(UrbanEnvironment),
                MakeEnumAccessor<EnvironmentType>(&OkumuraHataPropagationLossModel::m_environment),
                MakeEnumChecker(UrbanEnvironment,
                                "Urban",
                                SubUrbanEnvironment,
                                "SubUrban",
                                OpenAreasEnvironment,
                                "OpenAreas"))
            .AddAttribute(
                "CitySize",
                "Dimension of the city",
                EnumValue(LargeCity),
                MakeEnumAccessor<CitySize>(&OkumuraHataPropagationLossModel::m_citySize),
                MakeEnumChecker(SmallCity, "Small", MediumCity, "Medium", LargeCity, "Large"));
    return tid;
}

OkumuraHataPropagationLossModel::OkumuraHataPropagationLossModel()
    : PropagationLossModel()
{
}

OkumuraHataPropagationLossModel::~OkumuraHataPropagationLossModel()
{
}

double
OkumuraHataPropagationLossModel::GetLoss(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
    double loss = 0.0;
    double fmhz = m_frequency / 1e6;
    double log_fMhz = std::log10(fmhz);
    // In the Okumura Hata literature, the distance is expressed in units of kilometers
    // but other lengths are expressed in meters
    double distKm = a->GetDistanceFrom(b) / 1000.0;

    Vector aPosition = a->GetPosition();
    Vector bPosition = b->GetPosition();

    double hb = std::max(aPosition.z, bPosition.z);
    double hm = std::min(aPosition.z, bPosition.z);

    NS_ASSERT_MSG(hb > 0 && hm > 0, "nodes' height must be greater then 0");

    double log_hb = std::log10(hb);
    double log_aHeight = 13.82 * log_hb;
    double log_bHeight = 0.0;

    if (m_frequency <= 1.500e9)
    {
        // standard Okumura Hata
        // see eq. (4.4.1) in the COST 231 final report

        if (m_citySize == LargeCity)
        {
            if (fmhz < 200)
            {
                log_bHeight = 8.29 * std::pow(log10(1.54 * hm), 2) - 1.1;
            }
            else
            {
                log_bHeight = 3.2 * std::pow(log10(11.75 * hm), 2) - 4.97;
            }
        }
        else
        {
            log_bHeight = 0.8 + (1.1 * log_fMhz - 0.7) * hm - 1.56 * log_fMhz;
        }

        NS_LOG_INFO(this << " logf " << 26.16 * log_fMhz << " loga " << log_aHeight << " X "
                         << ((44.9 - (6.55 * log_hb)) * std::log10(distKm)) << " logb "
                         << log_bHeight);
        loss = 69.55 + (26.16 * log_fMhz) - log_aHeight +
               ((44.9 - (6.55 * log_hb)) * std::log10(distKm)) - log_bHeight;
        if (m_environment == SubUrbanEnvironment)
        {
            loss += -2 * (std::pow(std::log10(fmhz / 28), 2)) - 5.4;
        }
        else if (m_environment == OpenAreasEnvironment)
        {
            loss += -4.70 * std::pow(log_fMhz, 2) + 18.33 * log_fMhz - 40.94;
        }
    }
    else
    {
        // COST 231 Okumura model
        // see eq. (4.4.3) in the COST 231 final report

        double C = 0.0;

        if (m_citySize == LargeCity)
        {
            log_bHeight = 3.2 * std::pow(std::log10(11.75 * hm), 2);
            C = 3;
        }
        else
        {
            log_bHeight = (1.1 * log_fMhz - 0.7) * hm - (1.56 * log_fMhz - 0.8);
        }

        loss = 46.3 + (33.9 * log_fMhz) - log_aHeight +
               ((44.9 - (6.55 * log_hb)) * std::log10(distKm)) - log_bHeight + C;
    }
    return loss;
}

double
OkumuraHataPropagationLossModel::DoCalcRxPower(double txPowerDbm,
                                               Ptr<MobilityModel> a,
                                               Ptr<MobilityModel> b) const
{
    return (txPowerDbm - GetLoss(a, b));
}

int64_t
OkumuraHataPropagationLossModel::DoAssignStreams(int64_t stream)
{
    return 0;
}

} // namespace ns3
