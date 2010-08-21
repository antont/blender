/*
 * $Id$
 *
 * ***** BEGIN LGPL LICENSE BLOCK *****
 *
 * Copyright 2009 Jörg Hermann Müller
 *
 * This file is part of AudaSpace.
 *
 * AudaSpace is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AudaSpace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with AudaSpace.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ***** END LGPL LICENSE BLOCK *****
 */

#include "AUD_MixerFactory.h"
#include "AUD_IReader.h"

AUD_IReader* AUD_MixerFactory::getReader() const
{
	return m_factory->createReader();
}

AUD_MixerFactory::AUD_MixerFactory(AUD_IFactory* factory,
								   AUD_DeviceSpecs specs) :
	m_specs(specs), m_factory(factory)
{
}

AUD_DeviceSpecs AUD_MixerFactory::getSpecs() const
{
	return m_specs;
}

AUD_IFactory* AUD_MixerFactory::getFactory() const
{
	return m_factory;
}
