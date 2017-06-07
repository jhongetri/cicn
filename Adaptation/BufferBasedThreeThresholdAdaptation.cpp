/*
 * Copyright (c) 2017 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "BufferBasedThreeThresholdAdaptation.h"
#include<stdio.h>

using namespace dash::mpd;
using namespace libdash::framework::adaptation;
using namespace libdash::framework::input;
using namespace libdash::framework::mpd;

BufferBasedThreeThresholdAdaptation::BufferBasedThreeThresholdAdaptation(IMPD *mpd, IPeriod *period, IAdaptationSet *adaptationSet, bool isVid, struct AdaptationParameters *params) :
                  AbstractAdaptationLogic(mpd, period, adaptationSet, isVid)
{
	this->firstThreshold = params->BufferThreeThreshold_FirstThreshold;
	this->secondThreshold = params->BufferThreeThreshold_SecondThreshold;
	this->thirdThreshold = params->BufferThreeThreshold_ThirdThreshold;
	this->slackParam = params->BufferThreeThreshold_slackParameter;
    std::vector<IRepresentation* > representations = this->adaptationSet->GetRepresentation();
    this->representation = this->adaptationSet->GetRepresentation().at(0);
	this->multimediaManager = NULL;
	this->lastBufferFill = 0;
	this->bufferEOS = false;
	this->shouldAbort = false;
	this->isCheckedForReceiver = false;
	Debug("BufferRateBasedParams:\t%f\t%f\t%f\n",(double)this->firstThreshold/100, (double)secondThreshold/100, (double)thirdThreshold/100);
	Debug("Buffer Adaptation:	STARTED\n");
}

BufferBasedThreeThresholdAdaptation::~BufferBasedThreeThresholdAdaptation()
{
}

LogicType BufferBasedThreeThresholdAdaptation::getType()
{
    return adaptation::BufferBasedThreeThreshold;
}

bool BufferBasedThreeThresholdAdaptation::isUserDependent()
{
	return false;
}

bool BufferBasedThreeThresholdAdaptation::isRateBased()
{
	return true;
}

bool BufferBasedThreeThresholdAdaptation::isBufferBased()
{
	return true;
}

void BufferBasedThreeThresholdAdaptation::setMultimediaManager(viper::managers::IMultimediaManagerBase *_mmManager)
{
	this->multimediaManager = _mmManager;
}

void BufferBasedThreeThresholdAdaptation::notifyBitrateChange()
{
	if(this->multimediaManager)
        if(this->multimediaManager->isStarted() && !this->multimediaManager->isStopping())
			if(this->isVideo)
                this->multimediaManager->setVideoQuality(this->period, this->adaptationSet, this->representation);
			else
                this->multimediaManager->setAudioQuality(this->period, this->adaptationSet, this->representation);
	//Should Abort is done here to avoid race condition with DASHReceiver::DoBuffering()
	if(this->shouldAbort)
	{
        this->multimediaManager->shouldAbort(this->isVideo);
	}
	this->shouldAbort = false;
}

uint64_t BufferBasedThreeThresholdAdaptation::getBitrate()
{
	return this->currentBitrate;
}

void BufferBasedThreeThresholdAdaptation::setBitrate(uint32_t bufferFill)
{
	uint32_t phi1, phi2;
	std::vector<IRepresentation *> representations;
	representations = this->adaptationSet->GetRepresentation();
	size_t i = 0;

	if(this->isCheckedForReceiver)
	{
		return;
	}
	this->isCheckedForReceiver = true;


	if(bufferFill < this->firstThreshold)
	{
		this->myQuality = 0;
	}
	else
	{
		if(bufferFill < this->secondThreshold)
		{
			if(this->currentBitrate >= this->instantBw)
			{
				if(this->myQuality > 0)
				{
					this->myQuality--;
				}
			}
		}
		else
		{
			if(bufferFill < this->thirdThreshold)
			{
			}
			else
			{// bufferLevel > thirdThreshold
				if(this->currentBitrate <= this->instantBw)
				{
					if(this->myQuality < representations.size() - 1)
						this->myQuality++;
				}
			}
		}
	}
	this->representation = representations.at(this->myQuality);
	this->currentBitrate = (uint64_t) this->representation->GetBandwidth();
	Debug("ADAPTATION_LOGIC:\tFor %s:\tlast_buffer: %f\tbuffer_level: %f, instantaneousBw: %lu, choice: %d\n",isVideo ? "video" : "audio",(double)lastBufferFill/100 , (double)bufferFill/100, this->instantBw, this->myQuality);
}

void BufferBasedThreeThresholdAdaptation::bitrateUpdate(uint64_t bps, uint32_t segNum)
{
	this->instantBw = bps;
}

void BufferBasedThreeThresholdAdaptation::onEOS(bool value)
{
	this->bufferEOS = value;
}

void BufferBasedThreeThresholdAdaptation::checkedByDASHReceiver()
{
	this->isCheckedForReceiver = false;
}
void BufferBasedThreeThresholdAdaptation::bufferUpdate(uint32_t bufferFill, int maxC)
{
    this->setBitrate(bufferFill);
    this->notifyBitrateChange();
}

void BufferBasedThreeThresholdAdaptation::dLTimeUpdate(double time)
{
}

