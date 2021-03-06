﻿/* This file is part of the ScriptDev2 Project. See AUTHORS file for Copyright information
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Boss_Jin'do the Hexxer
SD%Complete: 100
SDComment:
SDCategory: 祖尔格拉布
EndScriptData */

#include "precompiled.h"
#include "zulgurub.h"

enum
{
    SAY_AGGRO                   = -1309014,

    SPELL_HEX                   = 17172,                    // 妖术
    SPELL_BRAIN_WASH            = 24261,                    // 洗脑
    SPELL_BRAINWASH_TOTEM       = 24262,                    // 召唤洗脑图腾
    SPELL_DELUSIONS_OF_JINDO    = 24306,                    // 金度的欺骗
    SPELL_SHADE_OF_JINDO_PASSIVE= 24307,                    // 金度之影(被动)
    SPELL_SHADE_OF_JINDO        = 24308,                    // 召唤金度之影
    SPELL_POWERFULL_HEALING_WARD= 24309,                    // 强力治疗结界
    SPELL_HEALING_WARD_HEAL     = 24311,                    // 强力治疗结界

    // 仆从
    NPC_SACRIFICED_TROLL        = 14826,                    // 祭品巨魔
    NPC_SHADE_OF_JINDO          = 14986,                    // 金度之影
    NPC_POWERFULL_HEALING_WARD  = 14987,                    // 强力治疗结界
    NPC_BRAIN_WASH_TOTEM        = 15112,                    // 洗脑图腾

    MAX_SKELETONS               = 9,
};

static const float aPitTeleportLocs[4] =
{
    -11583.7783f, -1249.4278f, 77.5471f, 4.745f
};

struct boss_jindoAI : public ScriptedAI
{
    boss_jindoAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiHexTimer;
    uint32 m_uiBrainWashTotemTimer;
    uint32 m_uiDelusionsTimer;
    uint32 m_uiHealingWardTimer;
    uint32 m_uiTeleportTimer;

    void Reset() override
    {
        m_uiHexTimer                = 8000;
        m_uiBrainWashTotemTimer     = 20000;
        m_uiDelusionsTimer          = 10000;
        m_uiHealingWardTimer        = 16000;
        m_uiTeleportTimer           = 5000;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        DoScriptText(SAY_AGGRO, m_creature);
    }

    void JustSummoned(Creature* pSummoned) override
    {
        if (pSummoned->GetEntry() == NPC_BRAIN_WASH_TOTEM)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                { pSummoned->CastSpell(pTarget, SPELL_BRAIN_WASH, true); }
        }
    }

    void SummonedCreatureJustDied(Creature* pSummoned) override
    {
        if (pSummoned->GetEntry() == NPC_POWERFULL_HEALING_WARD)
            { m_uiHealingWardTimer = 15000; }
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            { return; }

        // 妖术
        if (m_uiHexTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_HEX) == CAST_OK)
                { m_uiHexTimer = urand(12000, 20000); }
        }
        else
            { m_uiHexTimer -= uiDiff; }

        // 召唤洗脑图腾
        if (m_uiBrainWashTotemTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_BRAINWASH_TOTEM) == CAST_OK)
                { m_uiBrainWashTotemTimer = urand(18000, 26000); }
        }
        else
            { m_uiBrainWashTotemTimer -= uiDiff; }

        // 金度的欺骗
        if (m_uiDelusionsTimer < uiDiff)
        {
            Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1);
            if (!pTarget)
                { pTarget = m_creature->getVictim(); }
            if (DoCastSpellIfCan(pTarget, SPELL_DELUSIONS_OF_JINDO) == CAST_OK)
            {
                float fX, fY, fZ;
                m_creature->GetRandomPoint(pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), 5.0f, fX, fY, fZ);
                if (Creature* pSummoned = m_creature->SummonCreature(NPC_SHADE_OF_JINDO, fX, fY, fZ, 0, TEMPSUMMON_TIMED_OOC_DESPAWN, 15000))
                    { pSummoned->CastSpell(pSummoned, SPELL_SHADE_OF_JINDO_PASSIVE, true); }
                m_uiDelusionsTimer = urand(4000, 12000);
            }
        }
        else
            { m_uiDelusionsTimer -= uiDiff; }

        // 强力治疗结界
        if (m_uiHealingWardTimer)
        {
            if (m_uiHealingWardTimer <= uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_POWERFULL_HEALING_WARD) == CAST_OK)
                    { m_uiHealingWardTimer = 0; }
            }
            else
                { m_uiHealingWardTimer -= uiDiff; }
        }

        // 传送
        if (m_uiTeleportTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                // 不传送被洗脑的玩家
                if (pTarget->HasAura(SPELL_BRAIN_WASH))
                    { m_uiTeleportTimer = 1000; }
                else
                {
                    DoTeleportPlayer(pTarget, aPitTeleportLocs[0], aPitTeleportLocs[1], aPitTeleportLocs[2], aPitTeleportLocs[3]);
                    // 召唤9个祭品巨魔
                    float fX, fY, fZ;
                    for (uint8 i = 0; i < MAX_SKELETONS; ++i)
                    {
                        m_creature->GetRandomPoint(aPitTeleportLocs[0], aPitTeleportLocs[1], aPitTeleportLocs[2], 4.0f, fX, fY, fZ);
                        if (Creature* pSummoned = m_creature->SummonCreature(NPC_SACRIFICED_TROLL, fX, fY, fZ, 0.0f, TEMPSUMMON_TIMED_OOC_DESPAWN, 15000))
                            { pSummoned->AI()->AttackStart(pTarget); }
                    }
                    m_uiTeleportTimer = urand(15000, 23000);
                }
            }
        }
        else
            { m_uiTeleportTimer -= uiDiff; }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_jindo(Creature* pCreature)
{
    return new boss_jindoAI(pCreature);
}

void AddSC_boss_jindo()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_jindo";
    pNewScript->GetAI = &GetAI_boss_jindo;
    pNewScript->RegisterSelf();
}
